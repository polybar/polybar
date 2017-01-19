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
  void fill(xcb_connection_t* c, xcb_drawable_t d, xcb_gcontext_t g, short int x, short int y, unsigned short int w, unsigned short int h) {
    fill(c, d, g, {x, y, w, h});
  }
}

POLYBAR_NS_END
