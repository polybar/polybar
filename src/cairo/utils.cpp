#include "cairo/utils.hpp"

#include <map>

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

  /**
   * @see <cairo/cairo.h>
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
} // namespace utils
} // namespace cairo

POLYBAR_NS_END
