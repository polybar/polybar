#pragma once

#include <xcb/xcb_icccm.h>

#include "common.hpp"

POLYBAR_NS

namespace icccm_util {
  string get_wm_name(xcb_connection_t* c, xcb_window_t w);
  pair<string, string> get_wm_class(xcb_connection_t* c, xcb_window_t w);
  string get_reply_string(xcb_icccm_get_text_property_reply_t* reply);

  void set_wm_name(xcb_connection_t* c, xcb_window_t w, const char* wmname, size_t l, const char* wmclass, size_t l2);
  void set_wm_protocols(xcb_connection_t* c, xcb_window_t w, vector<xcb_atom_t> flags);
  bool get_wm_urgency(xcb_connection_t* c, xcb_window_t w);

  void set_wm_size_hints(xcb_connection_t* c, xcb_window_t w, int x, int y, int width, int height);
} // namespace icccm_util

POLYBAR_NS_END
