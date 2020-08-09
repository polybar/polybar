#pragma once

#include <cairo/cairo-xcb.h>

#include <algorithm>
#include <cmath>
#include <deque>

#include "cairo/font.hpp"
#include "cairo/surface.hpp"
#include "cairo/types.hpp"
#include "cairo/utils.hpp"
#include "common.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "utils/color.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace cairo {
  /**
   * \brief Cairo context
   */
  class context {
   public:
    explicit context(const surface& surface, const logger& log) : m_c(cairo_create(surface)), m_log(log) {
      auto status = cairo_status(m_c);
      if (status != CAIRO_STATUS_SUCCESS) {
        throw application_error(sstream() << "cairo_status(): " << cairo_status_to_string(status));
      }
      cairo_set_antialias(m_c, CAIRO_ANTIALIAS_GOOD);
    }

    virtual ~context() {
      cairo_destroy(m_c);
    }

    operator cairo_t*() const {
      return m_c;
    }

    context& operator<<(const surface& s) {
      cairo_set_source_surface(m_c, s, 0.0, 0.0);
      return *this;
    }

    context& operator<<(cairo_operator_t o) {
      cairo_set_operator(m_c, o);
      return *this;
    }

    context& operator<<(cairo_pattern_t* s) {
      cairo_set_source(m_c, s);
      return *this;
    }

    context& operator<<(const unsigned int& c) {
      // clang-format off
      cairo_set_source_rgba(m_c,
        color_util::red_channel<unsigned char>(c) / 255.0,
        color_util::green_channel<unsigned char>(c) / 255.0,
        color_util::blue_channel<unsigned char>(c) / 255.0,
        color_util::alpha_channel<unsigned char>(c) / 255.0);
      // clang-format on
      return *this;
    }

    context& operator<<(const abspos& p) {
      if (p.clear) {
        cairo_new_path(m_c);
      }
      cairo_move_to(m_c, p.x, p.y);
      return *this;
    }

    context& operator<<(const relpos& p) {
      cairo_rel_move_to(m_c, p.x, p.y);
      return *this;
    }

    context& operator<<(const rgba& f) {
      cairo_set_source_rgba(m_c, f.r, f.g, f.b, f.a);
      return *this;
    }

    context& operator<<(const rect& f) {
      cairo_rectangle(m_c, f.x, f.y, f.w, f.h);
      return *this;
    }

    context& operator<<(const line& l) {
      struct line p {
        l.x1, l.y1, l.x2, l.y2, l.w
      };
      snap(&p.x1, &p.y1);
      snap(&p.x2, &p.y2);
      cairo_move_to(m_c, p.x1, p.y1);
      cairo_line_to(m_c, p.x2, p.y2);
      cairo_set_line_width(m_c, p.w);
      cairo_stroke(m_c);
      return *this;
    }

    context& operator<<(const translate& d) {
      cairo_translate(m_c, d.x, d.y);
      return *this;
    }

    context& operator<<(const linear_gradient& l) {
      auto stops = l.steps.size();
      if (stops >= 2) {
        auto pattern = cairo_pattern_create_linear(l.x1, l.y1, l.x2, l.y2);
        auto step = 1.0 / (stops - 1);
        auto offset = 0.0;
        for (auto&& color : l.steps) {
          // clang-format off
          cairo_pattern_add_color_stop_rgba(pattern, offset,
            color_util::red_channel<unsigned char>(color) / 255.0,
            color_util::green_channel<unsigned char>(color) / 255.0,
            color_util::blue_channel<unsigned char>(color) / 255.0,
            color_util::alpha_channel<unsigned char>(color) / 255.0);
          // clang-format on
          offset += step;
        }
        *this << pattern;
        cairo_pattern_destroy(pattern);
      }
      return *this;
    }

    context& operator<<(const rounded_corners& c) {
      double d = M_PI / 180.0;
      cairo_new_sub_path(m_c);
      cairo_arc(m_c, c.x + c.w - c.radius.top, c.y + c.radius.top, c.radius.top, -90 * d, 0 * d);
      cairo_arc(m_c, c.x + c.w - c.radius.bottom, c.y + c.h - c.radius.bottom, c.radius.bottom, 0 * d, 90 * d);
      cairo_arc(m_c, c.x + c.radius.bottom, c.y + c.h - c.radius.bottom, c.radius.bottom, 90 * d, 180 * d);
      cairo_arc(m_c, c.x + c.radius.top, c.y + c.radius.top, c.radius.top, 180 * d, 270 * d);
      cairo_close_path(m_c);
      return *this;
    }

    context& operator<<(const textblock& t) {
      double x, y;
      position(&x, &y);

      // Prioritize the preferred font
      vector<shared_ptr<font>> fns(m_fonts.begin(), m_fonts.end());

      if (t.font > 0 && t.font <= std::distance(fns.begin(), fns.end())) {
        std::iter_swap(fns.begin(), fns.begin() + t.font - 1);
      }

      string utf8 = string(t.contents);
      utils::unicode_charlist chars;
      utils::utf8_to_ucs4((const unsigned char*)utf8.c_str(), chars);

      while (!chars.empty()) {
        auto remaining = chars.size();
        for (auto&& f : fns) {
          unsigned int matches = 0;

          // Match as many glyphs as possible if the default/preferred font
          // is being tested. Otherwise test one glyph at a time against
          // the remaining fonts. Roll back to the top of the font list
          // when a glyph has been found.
          if (f == fns.front() && (matches = f->match(chars)) == 0) {
            continue;
          } else if (f != fns.front() && (matches = f->match(chars.front())) == 0) {
            continue;
          }

          string subset;
          auto end = chars.begin();
          while (matches-- && end != chars.end()) {
            subset += utf8.substr(end->offset, end->length);
            end++;
          }

          // Use the font
          f->use();

          // Get subset extents
          cairo_text_extents_t extents;
          f->textwidth(subset, &extents);

          // Draw the background
          if (t.bg_rect.h != 0.0) {
            save();
            cairo_set_operator(m_c, t.bg_operator);
            *this << t.bg;
            cairo_rectangle(m_c, t.bg_rect.x + *t.x_advance, t.bg_rect.y + *t.y_advance,
                t.bg_rect.w + extents.x_advance, t.bg_rect.h);
            cairo_fill(m_c);
            restore();
          }

          // Render subset
          auto fontextents = f->extents();
          f->render(subset, x, y - (fontextents.descent / 2 - fontextents.height / 4) + f->offset());

          // Get updated position
          position(&x, nullptr);

          // Increase position
          *t.x_advance += extents.x_advance;
          *t.y_advance += extents.y_advance;

          chars.erase(chars.begin(), end);
          break;
        }

        if (chars.empty()) {
          break;
        } else if (remaining != chars.size()) {
          continue;
        }

        char unicode[6]{'\0'};
        utils::ucs4_to_utf8(unicode, chars.begin()->codepoint);
        m_log.warn("Dropping unmatched character %s (U+%04x) in '%s'", unicode, chars.begin()->codepoint, t.contents);
        utf8.erase(chars.begin()->offset, chars.begin()->length);
        for (auto&& c : chars) {
          c.offset -= chars.begin()->length;
        }
        chars.erase(chars.begin(), ++chars.begin());
      }

      return *this;
    }

    context& operator<<(shared_ptr<font>&& f) {
      m_fonts.emplace_back(forward<decltype(f)>(f));
      return *this;
    }

    context& save(bool save_point = false) {
      if (save_point) {
        m_points.emplace_front(make_pair<double, double>(0.0, 0.0));
        position(&m_points.front().first, &m_points.front().second);
      }
      m_activegroups++;
      cairo_save(m_c);
      return *this;
    }

    context& restore(bool restore_point = false) {
      if (!m_activegroups) {
        throw application_error("Unmatched calls to save/restore");
      }
      m_activegroups--;
      cairo_restore(m_c);
      if (restore_point && !m_points.empty()) {
        *this << abspos{m_points.front().first, m_points.front().first};
        m_points.pop_front();
      }
      return *this;
    }

    context& paint() {
      cairo_paint(m_c);
      return *this;
    }

    context& paint(double alpha) {
      cairo_paint_with_alpha(m_c, alpha);
      return *this;
    }

    context& fill(bool preserve = false) {
      if (preserve) {
        cairo_fill_preserve(m_c);
      } else {
        cairo_fill(m_c);
      }
      return *this;
    }

    context& mask(cairo_pattern_t* pattern) {
      cairo_mask(m_c, pattern);
      return *this;
    }

    context& pop(cairo_pattern_t** pattern) {
      *pattern = cairo_pop_group(m_c);
      return *this;
    }

    context& push() {
      cairo_push_group(m_c);
      return *this;
    }

    context& destroy(cairo_pattern_t** pattern) {
      cairo_pattern_destroy(*pattern);
      *pattern = nullptr;
      return *this;
    }

    context& clear(bool paint = true) {
      cairo_save(m_c);
      cairo_set_operator(m_c, CAIRO_OPERATOR_CLEAR);
      if (paint) {
        cairo_paint(m_c);
      } else {
        cairo_fill_preserve(m_c);
      }
      cairo_restore(m_c);
      return *this;
    }

    context& clip(bool preserve = false) {
      if (preserve) {
        cairo_clip_preserve(m_c);
      } else {
        cairo_clip(m_c);
        cairo_new_path(m_c);
      }
      return *this;
    }

    context& clip(const rect& r) {
      *this << r;
      return clip();
    }

    context& reset_clip() {
      cairo_reset_clip(m_c);
      return *this;
    }

    context& position(double* x, double* y = nullptr) {
      if (cairo_has_current_point(m_c)) {
        double x_, y_;
        x = x != nullptr ? x : &x_;
        y = y != nullptr ? y : &y_;
        cairo_get_current_point(m_c, x, y);
      }
      return *this;
    }

    context& snap(double* x, double* y) {
      cairo_user_to_device(m_c, x, y);
      *x = static_cast<int>(*x + 0.5);
      *y = static_cast<int>(*y + 0.5);
      return *this;
    }

   protected:
    cairo_t* m_c;
    const logger& m_log;
    vector<shared_ptr<font>> m_fonts;
    std::deque<pair<double, double>> m_points;
    int m_activegroups{0};
  };
}  // namespace cairo

POLYBAR_NS_END
