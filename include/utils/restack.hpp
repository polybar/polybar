#pragma once

#include <xcb/xcb.h>

#include "x11/connection.hpp"
#include "x11/ewmh.hpp"

POLYBAR_NS

namespace restack_util {
  void restack_relative(connection& conn, xcb_window_t win, xcb_window_t sibling, xcb_stack_mode_t stack_mode);
}

POLYBAR_NS_END
