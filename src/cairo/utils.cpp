#include <map>

#include "cairo/utils.hpp"

POLYBAR_NS

namespace cairo {
  namespace utils {

    // implementation : device_lock {{{

    device_lock::device_lock(cairo_device_t* device) {
      auto status = cairo_device_acquire(device);
      if (status == CAIRO_STATUS_SUCCESS) {
        m_device = device;
      }
    }
    device_lock::~device_lock() {
      cairo_device_release(m_device);
    }
    device_lock::operator bool() const {
      return m_device != nullptr;
    }
    device_lock::operator cairo_device_t*() const {
      return m_device;
    }

    // }}}
    // implementation : ft_face_lock {{{

    ft_face_lock::ft_face_lock(cairo_scaled_font_t* font) : m_font(font) {
      m_face = cairo_ft_scaled_font_lock_face(m_font);
    }
    ft_face_lock::~ft_face_lock() {
      cairo_ft_scaled_font_unlock_face(m_font);
    }
    ft_face_lock::operator FT_Face() const {
      return m_face;
    }

    // }}}
    // implementation : unicode_character {{{

    unicode_character::unicode_character() : codepoint(0), offset(0), length(0) {}

    // }}}

    /**
     * \see <cairo/cairo.h>
     */
    cairo_operator_t str2operator(const string& mode, cairo_operator_t fallback) {
      if (mode.empty()) {
        return fallback;
      }
      static std::map<string, cairo_operator_t> modes;
      if (modes.empty()) {
        modes["clear"s] = CAIRO_OPERATOR_CLEAR;
        modes["source"s] = CAIRO_OPERATOR_SOURCE;
        modes["over"s] = CAIRO_OPERATOR_OVER;
        modes["in"s] = CAIRO_OPERATOR_IN;
        modes["out"s] = CAIRO_OPERATOR_OUT;
        modes["atop"s] = CAIRO_OPERATOR_ATOP;
        modes["dest"s] = CAIRO_OPERATOR_DEST;
        modes["dest-over"s] = CAIRO_OPERATOR_DEST_OVER;
        modes["dest-in"s] = CAIRO_OPERATOR_DEST_IN;
        modes["dest-out"s] = CAIRO_OPERATOR_DEST_OUT;
        modes["dest-atop"s] = CAIRO_OPERATOR_DEST_ATOP;
        modes["xor"s] = CAIRO_OPERATOR_XOR;
        modes["add"s] = CAIRO_OPERATOR_ADD;
        modes["saturate"s] = CAIRO_OPERATOR_SATURATE;
        modes["multiply"s] = CAIRO_OPERATOR_MULTIPLY;
        modes["screen"s] = CAIRO_OPERATOR_SCREEN;
        modes["overlay"s] = CAIRO_OPERATOR_OVERLAY;
        modes["darken"s] = CAIRO_OPERATOR_DARKEN;
        modes["lighten"s] = CAIRO_OPERATOR_LIGHTEN;
        modes["color-dodge"s] = CAIRO_OPERATOR_COLOR_DODGE;
        modes["color-burn"s] = CAIRO_OPERATOR_COLOR_BURN;
        modes["hard-light"s] = CAIRO_OPERATOR_HARD_LIGHT;
        modes["soft-light"s] = CAIRO_OPERATOR_SOFT_LIGHT;
        modes["difference"s] = CAIRO_OPERATOR_DIFFERENCE;
        modes["exclusion"s] = CAIRO_OPERATOR_EXCLUSION;
        modes["hsl-hue"s] = CAIRO_OPERATOR_HSL_HUE;
        modes["hsl-saturation"s] = CAIRO_OPERATOR_HSL_SATURATION;
        modes["hsl-color"s] = CAIRO_OPERATOR_HSL_COLOR;
        modes["hsl-luminosity"s] = CAIRO_OPERATOR_HSL_LUMINOSITY;
      }
      auto it = modes.find(mode);
      return it != modes.end() ? it->second : fallback;
    }

    /**
     * \brief Create a UCS-4 codepoint from a utf-8 encoded string
     */
    bool utf8_to_ucs4(const unsigned char* src, unicode_charlist& result_list) {
      if (!src) {
        return false;
      }
      const unsigned char* first = src;
      while (*first) {
        int len = 0;
        unsigned long result = 0;
        if ((*first >> 7) == 0) {
          len = 1;
          result = *first;
        } else if ((*first >> 5) == 6) {
          len = 2;
          result = *first & 31;
        } else if ((*first >> 4) == 14) {
          len = 3;
          result = *first & 15;
        } else if ((*first >> 3) == 30) {
          len = 4;
          result = *first & 7;
        } else {
          return false;
        }
        const unsigned char* next;
        for (next = first + 1; *next && ((*next >> 6) == 2) && (next - first < len); next++) {
          result = result << 6;
          result |= *next & 63;
        }
        unicode_character uc_char;
        uc_char.codepoint = result;
        uc_char.offset = first - src;
        uc_char.length = next - first;
        result_list.push_back(uc_char);
        first = next;
      }
      return true;
    }

    /**
     * \brief Convert a UCS-4 codepoint to a utf-8 encoded string
     */
    size_t ucs4_to_utf8(char* utf8, unsigned int ucs) {
      if (ucs <= 0x7f) {
        *utf8 = ucs;
        return 1;
      } else if (ucs <= 0x07ff) {
        *(utf8++) = ((ucs >> 6) & 0xff) | 0xc0;
        *utf8 = (ucs & 0x3f) | 0x80;
        return 2;
      } else if (ucs <= 0xffff) {
        *(utf8++) = ((ucs >> 12) & 0x0f) | 0xe0;
        *(utf8++) = ((ucs >> 6) & 0x3f) | 0x80;
        *utf8 = (ucs & 0x3f) | 0x80;
        return 3;
      } else if (ucs <= 0x1fffff) {
        *(utf8++) = ((ucs >> 18) & 0x07) | 0xf0;
        *(utf8++) = ((ucs >> 12) & 0x3f) | 0x80;
        *(utf8++) = ((ucs >> 6) & 0x3f) | 0x80;
        *utf8 = (ucs & 0x3f) | 0x80;
        return 4;
      } else if (ucs <= 0x03ffffff) {
        *(utf8++) = ((ucs >> 24) & 0x03) | 0xf8;
        *(utf8++) = ((ucs >> 18) & 0x3f) | 0x80;
        *(utf8++) = ((ucs >> 12) & 0x3f) | 0x80;
        *(utf8++) = ((ucs >> 6) & 0x3f) | 0x80;
        *utf8 = (ucs & 0x3f) | 0x80;
        return 5;
      } else if (ucs <= 0x7fffffff) {
        *(utf8++) = ((ucs >> 30) & 0x01) | 0xfc;
        *(utf8++) = ((ucs >> 24) & 0x3f) | 0x80;
        *(utf8++) = ((ucs >> 18) & 0x3f) | 0x80;
        *(utf8++) = ((ucs >> 12) & 0x3f) | 0x80;
        *(utf8++) = ((ucs >> 6) & 0x3f) | 0x80;
        *utf8 = (ucs & 0x3f) | 0x80;
        return 6;
      } else {
        return 0;
      }
    }
  }
}

POLYBAR_NS_END
