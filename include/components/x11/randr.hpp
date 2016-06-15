#pragma once

#include "common.hpp"
#include "components/x11/connection.hpp"
#include "utils/memory.hpp"

LEMONBUDDY_NS

struct monitor {
  string name;
  int w = 0;
  int h = 0;
  int x = 0;
  int y = 0;
};

namespace randr_util {
  /**
   * Define monitor
   */
  inline shared_ptr<monitor> make_monitor(string name, int w, int h, int x, int y) {
    auto mon = make_shared<monitor>();
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
  inline vector<shared_ptr<monitor>> get_monitors(connection& conn, xcb_window_t root) {
    vector<shared_ptr<monitor>> monitors;
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
        [](shared_ptr<monitor>& m1, shared_ptr<monitor>& m2) -> bool {
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
