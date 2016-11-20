#pragma once

#include <xcb/xcb.h>

#include "common.hpp"

POLYBAR_NS

class connection;

namespace wm_util {
  void set_wmname(connection& conn, xcb_window_t win, string wm_name, string wm_class);
  void set_wmprotocols(connection& conn, xcb_window_t win, vector<xcb_atom_t> flags);
  void set_windowtype(connection& conn, xcb_window_t win, vector<xcb_atom_t> types);
  void set_wmstate(connection& conn, xcb_window_t win, vector<xcb_atom_t> states);
  void set_wmpid(connection& conn, xcb_window_t win, pid_t pid);
  void set_wmdesktop(connection& conn, xcb_window_t win, uint32_t desktop = -1u);

  void set_trayorientation(connection& conn, xcb_window_t win, uint32_t orientation);
  void set_trayvisual(connection& conn, xcb_window_t win, xcb_visualid_t visual);
}

POLYBAR_NS_END
