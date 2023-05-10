#pragma once

#include <cairo/cairo-ft.h>

#include "common.hpp"

POLYBAR_NS

namespace cairo {
namespace utils {
  /**
   * @brief RAII wrapper used acquire cairo_device_t
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
   * @brief RAII wrapper used to access the underlying
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
   * @see <cairo/cairo.h>
   */
  cairo_operator_t str2operator(const string& mode, cairo_operator_t fallback);
} // namespace utils
} // namespace cairo

POLYBAR_NS_END
