#pragma once

#include <xcb/xcb_ewmh.h>

#include "common.hpp"
#include "utils/memory.hpp"

POLYBAR_NS

struct position;

using ewmh_connection_t = memory_util::malloc_ptr_t<xcb_ewmh_connection_t>;

namespace ewmh_util {
  ewmh_connection_t initialize();

  bool supports(xcb_ewmh_connection_t* ewmh, xcb_atom_t atom, int screen = 0);

  string get_wm_name(xcb_ewmh_connection_t* conn, xcb_window_t win);
  string get_visible_name(xcb_ewmh_connection_t* conn, xcb_window_t win);
  string get_icon_name(xcb_ewmh_connection_t* conn, xcb_window_t win);
  string get_reply_string(xcb_ewmh_get_utf8_strings_reply_t* reply);

  vector<position> get_desktop_viewports(xcb_ewmh_connection_t* conn, int screen = 0);
  vector<string> get_desktop_names(xcb_ewmh_connection_t* conn, int screen = 0);
  uint32_t get_current_desktop(xcb_ewmh_connection_t* conn, int screen = 0);
  xcb_window_t get_active_window(xcb_ewmh_connection_t* conn, int screen = 0);

  void change_current_desktop(xcb_ewmh_connection_t* conn, uint32_t desktop);
}

POLYBAR_NS_END
