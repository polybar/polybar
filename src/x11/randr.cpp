#include "x11/randr.hpp"

LEMONBUDDY_NS

namespace randr_util {
  /**
   * Define monitor
   */
  monitor_t make_monitor(xcb_randr_output_t randr, string name, int w, int h, int x, int y) {
    monitor_t mon{new monitor_t::element_type{}};
    mon->randr_output = randr;
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
  vector<monitor_t> get_monitors(connection& conn, xcb_window_t root) {
    vector<monitor_t> monitors;
    auto outputs = conn.get_screen_resources(root).outputs();

    for (auto it = outputs.begin(); it != outputs.end(); it++) {
      try {
        auto info = conn.get_output_info(*it);
        if (info->connection != XCB_RANDR_CONNECTION_CONNECTED)
          continue;
        auto crtc = conn.get_crtc_info(info->crtc);
        string name{info.name().begin(), info.name().end()};
        monitors.emplace_back(make_monitor(*it, name, crtc->width, crtc->height, crtc->x, crtc->y));
      } catch (const xpp::randr::error::bad_crtc&) {
      } catch (const xpp::randr::error::bad_output&) {
      }
    }

    // use the same sort algo as lemonbar, to match the defaults
    sort(monitors.begin(), monitors.end(), [](monitor_t& m1, monitor_t& m2) -> bool {
      if (m1->x < m2->x || m1->y + m1->h <= m2->y)
        return 1;
      if (m1->x > m2->x || m1->y + m1->h > m2->y)
        return -1;
      return 0;
    });

    return monitors;
  }

  /**
   * Get backlight value range for given output
   */
  void get_backlight_range(connection& conn, const monitor_t& mon, backlight_values& dst) {
    auto reply = conn.query_output_property(mon->randr_output, Backlight);

    dst.min = 0;
    dst.max = 0;

    if (!reply->range || reply->length != 2)
      reply = conn.query_output_property(mon->randr_output, BACKLIGHT);
    if (!reply->range || reply->length != 2)
      return;

    auto range = reply.valid_values().begin();

    dst.min = static_cast<uint32_t>(*range++);
    dst.max = static_cast<uint32_t>(*range);
  }

  /**
   * Get backlight value for given output
   */
  void get_backlight_value(connection& conn, const monitor_t& mon, backlight_values& dst) {
    auto reply = conn.get_output_property(mon->randr_output, Backlight, XCB_ATOM_NONE, 0, 4, 0, 0);

    if (reply->num_items != 1 || reply->format != 32 || reply->type != XCB_ATOM_INTEGER)
      reply = conn.get_output_property(mon->randr_output, BACKLIGHT, XCB_ATOM_NONE, 0, 4, 0, 0);
    if (reply->num_items == 1 && reply->format == 32 && reply->type == XCB_ATOM_INTEGER)
      dst.val = *reinterpret_cast<uint32_t*>(xcb_randr_get_output_property_data(reply.get().get()));
    else
      dst.val = 0;
  }
}

LEMONBUDDY_NS_END
