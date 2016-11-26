#pragma once

#include <X11/Xlib-xcb.h>
#include <xcb/xcb_util.h>

#include "common.hpp"
#include "x11/randr.hpp"

POLYBAR_NS

class connection;
class config;

namespace xutils {
  xcb_connection_t* get_connection();

  uint32_t event_timer_ms(const config& conf, const xcb_button_press_event_t&);
  uint32_t event_timer_ms(const config& conf, const xcb_randr_notify_event_t&);

  void pack_values(uint32_t mask, const uint32_t* src, uint32_t* dest);
  void pack_values(uint32_t mask, const xcb_params_cw_t* src, uint32_t* dest);
  void pack_values(uint32_t mask, const xcb_params_gc_t* src, uint32_t* dest);
  void pack_values(uint32_t mask, const xcb_params_configure_window_t* src, uint32_t* dest);

  void visibility_notify(connection& conn, const xcb_window_t& win, xcb_visibility_t state);

  void compton_shadow_exclude(connection& conn, const xcb_window_t& win);
}

POLYBAR_NS_END
