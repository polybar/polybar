#pragma once

#include <xcb/xcb.h>

#include "x11/connection.hpp"
#include "x11/ewmh.hpp"

POLYBAR_NS

namespace restack_util {
  using params = std::pair<xcb_window_t, xcb_stack_mode_t>;

  void restack_relative(connection& conn, xcb_window_t win, xcb_window_t sibling, xcb_stack_mode_t stack_mode);
  bool are_siblings(connection& conn, xcb_window_t win, xcb_window_t sibling);
  params get_bottom_params(connection& conn, xcb_window_t bar_window);
  params get_generic_params(connection& conn, xcb_window_t bar_window);
}

POLYBAR_NS_END
