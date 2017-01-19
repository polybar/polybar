#pragma once

#include <xcb/xcb.h>

#include "common.hpp"

POLYBAR_NS

namespace draw_util {
  void fill(xcb_connection_t* c, xcb_drawable_t d, xcb_gcontext_t g, const xcb_rectangle_t rect);
  void fill(xcb_connection_t* c, xcb_drawable_t d, xcb_gcontext_t g, short int x, short int y, unsigned short int w, unsigned short int h);
}

POLYBAR_NS_END
