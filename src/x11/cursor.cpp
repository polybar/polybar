#include "x11/cursor.hpp"

#include "utils/scope.hpp"

POLYBAR_NS

namespace cursor_util {
  bool valid(const string& name) {
    return (cursors.find(name) != cursors.end());
  }

  bool set_cursor(xcb_connection_t* c, xcb_screen_t* screen, xcb_window_t w, const string& name) {
    if (!valid(name)) {
      throw std::runtime_error("Tried to set cursor to invalid name: '" + name + "'");
    }

    xcb_cursor_context_t* ctx;

    if (xcb_cursor_context_new(c, screen, &ctx) < 0) {
      return false;
    }

    scope_util::on_exit handler([&] { xcb_cursor_context_free(ctx); });

    xcb_cursor_t cursor = XCB_CURSOR_NONE;
    for (const auto& cursor_name : cursors.at(name)) {
      cursor = xcb_cursor_load_cursor(ctx, cursor_name.c_str());
      if (cursor != XCB_CURSOR_NONE) {
        break;
      }
    }

    if (cursor == XCB_CURSOR_NONE) {
      return false;
    }

    xcb_change_window_attributes(c, w, XCB_CW_CURSOR, &cursor);
    xcb_free_cursor(c, cursor);

    return true;
  }
} // namespace cursor_util
POLYBAR_NS_END
