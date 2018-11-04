#pragma once

#include <cairo/cairo-ft.h>
#include <list>

#include "common.hpp"

POLYBAR_NS

namespace cairo {
  namespace utils {
    /**
     * \brief RAII wrapper used acquire cairo_device_t
     */
    class device_lock {
     public:
      explicit device_lock(cairo_device_t* device);
      ~device_lock();
      operator bool() const;
      operator cairo_device_t*() const;

     private:
      cairo_device_t* m_device{nullptr};
    };

    /**
     * \brief RAII wrapper used to access the underlying
     * FT_Face of a scaled font face
     */
    class ft_face_lock {
     public:
      explicit ft_face_lock(cairo_scaled_font_t* font);
      ~ft_face_lock();
      operator FT_Face() const;

     private:
      cairo_scaled_font_t* m_font;
      FT_Face m_face;
    };

    /**
     * \brief Unicode character containing converted codepoint
     * and details on where its position in the source string
     */
    struct unicode_character {
      explicit unicode_character();
      unsigned long codepoint;
      int offset;
      int length;
    };
    using unicode_charlist = std::list<unicode_character>;

    /**
     * \see <cairo/cairo.h>
     */
    cairo_operator_t str2operator(const string& mode, cairo_operator_t fallback);

    /**
     * \brief Create a UCS-4 codepoint from a utf-8 encoded string
     */
    bool utf8_to_ucs4(const unsigned char* src, unicode_charlist& result_list);

    /**
     * \brief Convert a UCS-4 codepoint to a utf-8 encoded string
     */
    size_t ucs4_to_utf8(char* utf8, unsigned int ucs);
  }
}

POLYBAR_NS_END
