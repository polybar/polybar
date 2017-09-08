#include "x11/cursor.hpp"

POLYBAR_NS

namespace cursor_util {
  bool set_cursor(xcb_connection_t *c, xcb_screen_t *screen, xcb_window_t w, string name) {
    xcb_cursor_t cursor = XCB_CURSOR_NONE;
    xcb_cursor_context_t *ctx;

    if (xcb_cursor_context_new(c, screen, &ctx) < 0) {
      return false;
    }

    const vector<string> *name_list;
    if (string_util::compare("pointer", name)) {
      name_list = &pointer_names;
    } else if (string_util::compare("ns-resize", name)) {
      name_list = &ns_resize_names;
    } else {
      name_list = &default_names;
    }

    for (auto&& cursor_name : *name_list) {
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
