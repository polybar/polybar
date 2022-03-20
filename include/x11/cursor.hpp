#pragma once

#include "settings.hpp"

#if not WITH_XCURSOR
#error "Not built with support for xcb-cursor..."
#endif

#include <xcb/xcb_cursor.h>

#include "common.hpp"
#include "utils/string.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

namespace cursor_util {
  static const std::map<string, vector<string>> cursors = {
      {"pointer", {"pointing_hand", "pointer", "hand", "hand1", "hand2", "e29285e634086352946a0e7090d73106",
                      "9d800788f1b08800ae810202380a0822"}},
      {"default", {"left_ptr", "arrow", "dnd-none", "op_left_arrow"}},
      {"ns-resize", {"size_ver", "sb_v_double_arrow", "v_double_arrow", "n-resize", "s-resize", "col-resize",
                        "top_side", "bottom_side", "base_arrow_up", "base_arrow_down", "based_arrow_down",
                        "based_arrow_up", "00008160000006810000408080010102"}}};
  bool valid(const string& name);
  bool set_cursor(xcb_connection_t* c, xcb_screen_t* screen, xcb_window_t w, const string& name);
} // namespace cursor_util

POLYBAR_NS_END
