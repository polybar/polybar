#pragma once

#include <xcb/xcb.h>

#include "common.hpp"

LEMONBUDDY_NS

class connection;

namespace graphics_util {
  struct root_pixmap {
    uint8_t depth{0};
    uint16_t width{0};
    uint16_t height{0};
    int16_t x{0};
    int16_t y{0};
    xcb_pixmap_t pixmap{0};
  };

  void get_root_pixmap(connection& conn, root_pixmap* rpix);

  void simple_gc(connection& conn, xcb_drawable_t drawable, xcb_gcontext_t* gc);
  void simple_pixmap(connection& conn, xcb_window_t dst, int w, int h, xcb_pixmap_t* pixmap);
}

LEMONBUDDY_NS_END
