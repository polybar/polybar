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
   * @brief Cairo context
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

    context& operator<<(const displace& d) {
      cairo_set_operator(m_c, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_surface(m_c, cairo_get_target(m_c), d.x - d.w, d.y);
      cairo_rectangle(m_c, d.x - d.w, d.y, d.x + d.dx, d.y + d.dy);
      cairo_fill(m_c);
      cairo_surface_flush(cairo_get_target(m_c));
      return *this;
    }

    context& operator<<(const linear_gradient& l) {
      if (l.steps.size() >= 2) {
        auto pattern = cairo_pattern_create_linear(l.x1, l.y1, l.x2, l.y2);
        *this << pattern;
        auto stops = l.steps.size();
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
        cairo_pattern_destroy(pattern);
      }
      return *this;
    }

    context& operator<<(const rounded_corners& c) {
      double radius = c.radius / 1.0;
      double d = M_PI / 180.0;
      cairo_new_sub_path(m_c);
      cairo_arc(m_c, c.x + c.w - radius, c.y + radius, radius, -90 * d, 0 * d);
      cairo_arc(m_c, c.x + c.w - radius, c.y + c.h - radius, radius, 0 * d, 90 * d);
      cairo_arc(m_c, c.x + radius, c.y + c.h - radius, radius, 90 * d, 180 * d);
      cairo_arc(m_c, c.x + radius, c.y + radius, radius, 180 * d, 270 * d);
      cairo_close_path(m_c);
      return *this;
    }

    context& operator<<(const textblock& t) {
      double x, y;
      position(&x, &y);

      // Sort the fontlist so that the
      // preferred font gets prioritized
      vector<shared_ptr<font>> fns(m_fonts.begin(), m_fonts.end());
      std::sort(fns.begin(), fns.end(), [&](const shared_ptr<font>& a, const shared_ptr<font>&) {
        if (t.font > 0 && std::distance(fns.begin(), std::find(fns.begin(), fns.end(), a)) == t.font - 1) {
          return -1;
        } else {
          return 0;
        }
      });

      string utf8 = string(t.contents);
      utils::unicode_charlist chars;
      utils::utf8_to_ucs4((const unsigned char*)utf8.c_str(), chars);

      while (!chars.empty()) {
        auto remaining = chars.size();
        for (auto&& f : fns) {
          auto matches = f->match(chars);
          if (!matches) {
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

          save(true);
          {
            *this << t.bg;
            cairo_set_operator(m_c, static_cast<cairo_operator_t>(t.bg_operator));
            cairo_rectangle(m_c, t.bg_rect.x + *t.x_advance, t.bg_rect.y + *t.y_advance,
                t.bg_rect.w + extents.x_advance, t.bg_rect.h);
            cairo_fill(m_c);
          }
          restore(true);

          // Render subset
          auto fontextents = f->extents();
          f->render(subset, x, y - (extents.height / 2.0 + extents.y_bearing + fontextents.descent) + f->offset());

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
        m_log.warn("Dropping unmatched character %s (U+%04x)", unicode, chars.begin()->codepoint);
        utf8.erase(chars.begin()->offset, chars.begin()->length);
        for (auto&& c : chars) {
          c.offset -= chars.begin()->length;
        }
        chars.erase(chars.begin(), ++chars.begin());
      }

      // *this << abspos{x, y};

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
      cairo_save(m_c);
      return *this;
    }

    context& restore(bool restore_point = false) {
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

    context& clear() {
      cairo_save(m_c);
      cairo_set_operator(m_c, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba(m_c, 0.0, 0.0, 0.0, 0.0);
      cairo_paint(m_c);
      cairo_restore(m_c);
      return *this;
    }

    context& clip(const rect& r) {
      *this << r;
      cairo_clip(m_c);
      cairo_new_path(m_c);
      return *this;
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
      *x = ((int)*x + 0.5);
      *y = ((int)*y + 0.5);
      return *this;
    }

   protected:
    cairo_t* m_c;
    const logger& m_log;
    vector<shared_ptr<font>> m_fonts;
    std::deque<pair<double, double>> m_points;
  };
}

POLYBAR_NS_END
