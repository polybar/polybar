#include "x11/draw.hpp"
#include "utils/color.hpp"

POLYBAR_NS

namespace draw_util {
  /**
   * Fill region of drawable with color defined by gcontext
   */
  void fill(xcb_connection_t* c, xcb_drawable_t d, xcb_gcontext_t g, const xcb_rectangle_t rect) {
    xcb_poly_fill_rectangle(c, d, g, 1, &rect);
  }

  /**
   * Fill region of drawable with color defined by gcontext
   */
  void fill(xcb_connection_t* c, xcb_drawable_t d, xcb_gcontext_t g, int16_t x, int16_t y, uint16_t w, uint16_t h) {
    fill(c, d, g, {x, y, w, h});
  }
}

POLYBAR_NS_END
