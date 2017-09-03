#pragma once

//#if not WITH_CURSOR
//#error "Not built with support for xcb-cursor..."
//#endif

#include <xcb/xcb_cursor.h>

#include "common.hpp"
#include "x11/connection.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace cursor_util {
  static const vector<string> pointer_names {"pointing_hand", "pointer", "hand", "hand1", "hand2", "e29285e634086352946a0e7090d73106", "9d800788f1b08800ae810202380a0822"};
  static const vector<string> arrow_names {"left_ptr", "arrow", "dnd-none", "op_left_arrow"};
  static const vector<string> ns_resize_names {"size_ver", "sb_v_double_arrow", "v_double_arrow", "n-resize", "s-resize", "col-resize", "top_side", "bottom_side", "base_arrow_up", "base_arrow_down", "based_arrow_down", "based_arrow_up", "00008160000006810000408080010102"};
  bool set_cursor(xcb_connection_t *c, xcb_screen_t *screen, xcb_window_t w, string name);
}

POLYBAR_NS_END
