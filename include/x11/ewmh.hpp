#pragma once

#include <xcb/xcb_ewmh.h>

#include "common.hpp"
#include "x11/connection.hpp"

LEMONBUDDY_NS

namespace ewmh_util {
  bool setup(connection& conn, xcb_ewmh_connection_t* dst);
  bool supports(xcb_ewmh_connection_t* ewmh, xcb_atom_t atom);

  xcb_window_t get_active_window(xcb_ewmh_connection_t* conn);

  string get_visible_name(xcb_ewmh_connection_t* conn, xcb_window_t win);
  string get_icon_name(xcb_ewmh_connection_t* conn, xcb_window_t win);
  string get_reply_string(xcb_ewmh_get_utf8_strings_reply_t* reply);
}

LEMONBUDDY_NS_END
