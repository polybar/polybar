#pragma once

#include "config.hpp"

#ifndef ENABLE_RANDR_EXT
#error "RandR extension is disabled..."
#endif

#include "common.hpp"
#include "utils/memory.hpp"
#include "x11/connection.hpp"

LEMONBUDDY_NS

struct backlight_values {
  uint32_t min = 0;
  uint32_t max = 0;
  uint32_t val = 0;
};

struct randr_output {
  xcb_randr_output_t randr_output;
  string name;
  int w = 0;
  int h = 0;
  int x = 0;
  int y = 0;
  backlight_values backlight;
};

using monitor_t = shared_ptr<randr_output>;

namespace randr_util {
  monitor_t make_monitor(xcb_randr_output_t randr, string name, int w, int h, int x, int y);
  vector<monitor_t> get_monitors(connection& conn, xcb_window_t root);

  void get_backlight_range(connection& conn, const monitor_t& mon, backlight_values& dst);
  void get_backlight_value(connection& conn, const monitor_t& mon, backlight_values& dst);
}

LEMONBUDDY_NS_END
