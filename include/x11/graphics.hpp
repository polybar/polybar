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

  bool create_window(connection& conn, xcb_window_t* win, int16_t x = 0, int16_t y = 0, uint16_t w = 1, uint16_t h = 1, xcb_window_t root = 0);
  bool create_pixmap(connection& conn, xcb_drawable_t dst, uint16_t w, uint16_t h, xcb_pixmap_t* pixmap);
  bool create_pixmap(connection& conn, xcb_drawable_t dst, uint16_t w, uint16_t h, uint8_t d, xcb_pixmap_t* pixmap);
  bool create_gc(connection& conn, xcb_drawable_t drawable, xcb_gcontext_t* gc);

  bool get_root_pixmap(connection& conn, root_pixmap* rpix);
}

LEMONBUDDY_NS_END
