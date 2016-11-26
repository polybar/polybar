#pragma once

#include <xcb/xcb.h>

#include "common.hpp"

POLYBAR_NS

namespace wm_util {
  void set_wmname(xcb_connection_t* conn, xcb_window_t win, const string& wm_name, const string& wm_class);
  void set_wmprotocols(xcb_connection_t* conn, xcb_window_t win, vector<xcb_atom_t> flags);
  void set_windowtype(xcb_connection_t* conn, xcb_window_t win, vector<xcb_atom_t> types);
  void set_wmstate(xcb_connection_t* conn, xcb_window_t win, vector<xcb_atom_t> states);
  void set_wmpid(xcb_connection_t* conn, xcb_window_t win, pid_t pid);
  void set_wmdesktop(xcb_connection_t* conn, xcb_window_t win, uint32_t desktop = -1u);

  void set_trayorientation(xcb_connection_t* conn, xcb_window_t win, uint32_t orientation);
  void set_trayvisual(xcb_connection_t* conn, xcb_window_t win, xcb_visualid_t visual);
}

POLYBAR_NS_END
