#pragma once

#include <xcb/xcb.h>

#include "common.hpp"

POLYBAR_NS

namespace wm_util {
  void set_wm_name(xcb_connection_t* conn, xcb_window_t win, const string& wm_name, const string& wm_class);
  void set_wm_protocols(xcb_connection_t* conn, xcb_window_t win, vector<xcb_atom_t> flags);
  void set_wm_window_type(xcb_connection_t* conn, xcb_window_t win, vector<xcb_atom_t> types);
  void set_wm_state(xcb_connection_t* conn, xcb_window_t win, vector<xcb_atom_t> states);
  void set_wm_pid(xcb_connection_t* conn, xcb_window_t win, pid_t pid);
  void set_wm_desktop(xcb_connection_t* conn, xcb_window_t win, uint32_t desktop = -1u);
  void set_wm_window_opacity(xcb_connection_t* conn, xcb_window_t win, uint64_t values);
}

POLYBAR_NS_END
