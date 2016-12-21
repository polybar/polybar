#pragma once

#include <X11/Xlib-xcb.h>
#include <xcb/xcb_util.h>

#include "common.hpp"

POLYBAR_NS

class connection;
class config;

namespace xutils {
  struct xcb_connection_deleter {
    void operator()(xcb_connection_t* c) {
      xcb_disconnect(c);
    }
  };

  shared_ptr<xcb_connection_t> get_connection();
  int get_connection_fd();

  void pack_values(uint32_t mask, const uint32_t* src, uint32_t* dest);
  void pack_values(uint32_t mask, const xcb_params_cw_t* src, uint32_t* dest);
  void pack_values(uint32_t mask, const xcb_params_gc_t* src, uint32_t* dest);
  void pack_values(uint32_t mask, const xcb_params_configure_window_t* src, uint32_t* dest);

  void visibility_notify(connection& conn, const xcb_window_t& win, xcb_visibility_t state);

  void compton_shadow_exclude(connection& conn, const xcb_window_t& win);
}

POLYBAR_NS_END
