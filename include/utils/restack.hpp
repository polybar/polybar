#pragma once

#include <xcb/xcb.h>

#include "x11/connection.hpp"
#include "x11/ewmh.hpp"

POLYBAR_NS

namespace restack_util {
using params = std::pair<xcb_window_t, xcb_stack_mode_t>;

static constexpr params NONE_PARAMS = {XCB_NONE, XCB_STACK_MODE_ABOVE};

void restack_relative(connection& conn, xcb_window_t win, xcb_window_t sibling, xcb_stack_mode_t stack_mode);
string stack_mode_to_string(xcb_stack_mode_t mode);
bool are_siblings(connection& conn, xcb_window_t win, xcb_window_t sibling);
params get_bottom_params(connection& conn, xcb_window_t bar_window);
params get_ewmh_params(connection& conn);
params get_generic_params(connection& conn, xcb_window_t bar_window);
} // namespace restack_util

POLYBAR_NS_END
