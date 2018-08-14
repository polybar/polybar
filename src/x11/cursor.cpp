#include "x11/cursor.hpp"

POLYBAR_NS

namespace cursor_util {
  bool valid(string name) {
    return (cursors.find(name) != cursors.end());
  }

  bool set_cursor(xcb_connection_t *c, xcb_screen_t *screen, xcb_window_t w, string name) {
    xcb_cursor_t cursor = XCB_CURSOR_NONE;
    xcb_cursor_context_t *ctx;

    if (xcb_cursor_context_new(c, screen, &ctx) < 0) {
      return false;
    }
    for (auto&& cursor_name : cursors.at(name)) {
      cursor = xcb_cursor_load_cursor(ctx, cursor_name.c_str());
      if (cursor != XCB_CURSOR_NONE)
        break;
    }
    xcb_change_window_attributes(c, w, XCB_CW_CURSOR, &cursor);
    xcb_cursor_context_free(ctx);
    return true;
  }
}
POLYBAR_NS_END
