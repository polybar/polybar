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
#include "errors.hpp"
#include "utils/color.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace cairo {
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

    context& operator<<(const linear_gradient& l) {
      if (l.steps.size() >= 2) {
        auto pattern = cairo_pattern_create_linear(l.x0, l.y0, l.x1, l.y1);
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

    context& operator<<(const textblock& t) {
      // Sort the fontlist so that the preferred font is tested first
      auto& fns = m_fonts;
      std::sort(fns.begin(), fns.end(), [&](const unique_ptr<font>& a, const unique_ptr<font>&) {
        if (t.fontindex > 0 && std::distance(fns.begin(), std::find(fns.begin(), fns.end(), a)) == t.fontindex - 1) {
          return -1;
        } else {
          return 0;
        }
      });

      string utf8 = string(t.contents);
      unicode_charlist chars;
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

          f->use(m_c);

          cairo_text_extents_t extents;
          f->textwidth(utf8, &extents);

          save(); // encapsulate the y pos update
          *this << relpos{0.0, extents.height / 2.0 - f->extents().descent + f->offset()};
          f->render(subset);
          restore();

          *this << relpos{extents.width, 0.0};

          chars.erase(chars.begin(), end);
          break;
        }

        if (chars.empty()) {
          break;
        } else if (remaining != chars.size()) {
          continue;
        }

        char unicode[5]{'\0'};
        utils::ucs4_to_utf8(unicode, chars.begin()->codepoint);
        m_log.warn("Dropping unmatched character %s (U+%04x)", unicode, chars.begin()->codepoint);
        utf8.erase(chars.begin()->offset, chars.begin()->length);
        for (auto&& c : chars) {
          c.offset -= chars.begin()->length;
        }
        chars.erase(chars.begin(), ++chars.begin());
      }

      return *this;
    }

    context& operator<<(unique_ptr<font>&& f) {
      m_fonts.emplace_back(forward<decltype(f)>(f));
      return *this;
    }

    context& save(bool save_point = true) {
      if (save_point) {
        m_points.emplace_front(make_pair<double, double>(0.0, 0.0));
        position(&m_points.front().first, &m_points.front().second);
      }
      cairo_save(m_c);
      return *this;
    }

    context& restore(bool restore_point = true) {
      if (restore_point) {
        *this << abspos{m_points.front().first, m_points.front().first};
        m_points.pop_front();
      }
      cairo_restore(m_c);
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

    context& fill() {
      cairo_fill(m_c);
      return *this;
    }

    context& clip(const rect& r) {
      *this << r;
      cairo_clip(m_c);
      return *this;
    }

    context& reset_clip() {
      cairo_reset_clip(m_c);
      return *this;
    }

    context& position(double* x, double* y) {
      *x = 0.0;
      *y = 0.0;
      if (cairo_has_current_point(m_c)) {
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
    vector<unique_ptr<font>> m_fonts;
    std::deque<pair<double, double>> m_points;
  };
}

POLYBAR_NS_END
