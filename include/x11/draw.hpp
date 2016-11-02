#pragma once

#include "common.hpp"
#include "x11/connection.hpp"

LEMONBUDDY_NS

namespace draw_util {
  void fill(connection& c, xcb_drawable_t d, xcb_gcontext_t g, int16_t x, int16_t y, uint16_t w,
      uint16_t h);

  xcb_void_cookie_t xcb_poly_text_16_patched(xcb_connection_t* conn, xcb_drawable_t d,
      xcb_gcontext_t gc, int16_t x, int16_t y, uint8_t len, uint16_t* str);
}

LEMONBUDDY_NS_END
