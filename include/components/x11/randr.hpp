#pragma once

#include "common.hpp"
#include "components/x11/connection.hpp"
#include "utils/memory.hpp"

LEMONBUDDY_NS

struct randr_output {
  string name;
  int w = 0;
  int h = 0;
  int x = 0;
  int y = 0;
};

using monitor_t = shared_ptr<randr_output>;

namespace randr_util {
  /**
   * Define monitor
   */
  inline monitor_t make_monitor(string name, int w, int h, int x, int y) {
    monitor_t mon{new monitor_t::element_type{}};
    mon->name = name;
    mon->x = x;
    mon->y = y;
    mon->h = h;
    mon->w = w;
    return mon;
  }

  /**
   * Create a list of all available randr outputs
   */
  inline vector<monitor_t> get_monitors(connection& conn, xcb_window_t root) {
    vector<monitor_t> monitors;
    auto outputs = conn.get_screen_resources(root).outputs();

    for (auto it = outputs.begin(); it != outputs.end(); it++) {
      auto info = conn.get_output_info(*it);

      if (info->connection != XCB_RANDR_CONNECTION_CONNECTED)
        continue;

      auto crtc = conn.get_crtc_info(info->crtc);
      string name{info.name().begin(), info.name().end()};

      monitors.emplace_back(make_monitor(name, crtc->width, crtc->height, crtc->x, crtc->y));
    }

    // use the same sort algo as lemonbar, to match the defaults
    sort(monitors.begin(), monitors.end(),
        [](monitor_t& m1, monitor_t& m2) -> bool {
          if (m1->x < m2->x || m1->y + m1->h <= m2->y)
            return 1;
          if (m1->x > m2->x || m1->y + m1->h > m2->y)
            return -1;
          return 0;
        });

    return monitors;
  }
}

LEMONBUDDY_NS_END
