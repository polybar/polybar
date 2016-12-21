#pragma once

#include <xcb/xcb.h>

#include "common.hpp"

POLYBAR_NS

namespace draw_util {
  void fill(xcb_connection_t* c, xcb_drawable_t d, xcb_gcontext_t g, const xcb_rectangle_t rect);
  void fill(xcb_connection_t* c, xcb_drawable_t d, xcb_gcontext_t g, int16_t x, int16_t y, uint16_t w, uint16_t h);
}

POLYBAR_NS_END
