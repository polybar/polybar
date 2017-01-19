#pragma once

#include <xcb/xcb.h>

#include "common.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

namespace graphics_util {
  struct root_pixmap {
    unsigned char depth{0};
    unsigned short int width{0};
    unsigned short int height{0};
    short int x{0};
    short int y{0};
    xcb_pixmap_t pixmap{0};
  };

  bool create_window(connection& conn, xcb_window_t* win, short int x = 0, short int y = 0, unsigned short int w = 1, unsigned short int h = 1,
      xcb_window_t root = 0);
  bool create_pixmap(connection& conn, xcb_drawable_t dst, unsigned short int w, unsigned short int h, xcb_pixmap_t* pixmap);
  bool create_pixmap(connection& conn, xcb_drawable_t dst, unsigned short int w, unsigned short int h, unsigned char d, xcb_pixmap_t* pixmap);
  bool create_gc(connection& conn, xcb_drawable_t drawable, xcb_gcontext_t* gc);

  bool get_root_pixmap(connection& conn, root_pixmap* rpix);
}

POLYBAR_NS_END
