#include "x11/randr.hpp"

POLYBAR_NS

namespace randr_util {
  /**
   * Define monitor
   */
  monitor_t make_monitor(xcb_randr_output_t randr, string name, uint16_t w, uint16_t h, int16_t x, int16_t y) {
    monitor_t mon{new monitor_t::element_type{}};
    mon->output = randr;
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
  vector<monitor_t> get_monitors(connection& conn, xcb_window_t root, bool connected_only) {
    vector<monitor_t> monitors;
    auto outputs = conn.get_screen_resources(root).outputs();

    for (auto it = outputs.begin(); it != outputs.end(); it++) {
      try {
        auto info = conn.get_output_info(*it);
        if (info->crtc == XCB_NONE)
          continue;
        if (connected_only && info->connection != XCB_RANDR_CONNECTION_CONNECTED)
          continue;
        auto crtc = conn.get_crtc_info(info->crtc);
        string name{info.name().begin(), info.name().end()};
        monitors.emplace_back(make_monitor(*it, name, crtc->width, crtc->height, crtc->x, crtc->y));
      } catch (const xpp::randr::error::bad_crtc&) {
      } catch (const xpp::randr::error::bad_output&) {
      }
    }

    // clang-format off
    const auto remove_monitor = [&](const monitor_t& monitor) {
      monitors.erase(find(monitors.begin(), monitors.end(), monitor));
    };
    // clang-format on

    for (auto m = monitors.rbegin(); m != monitors.rend(); m++) {
      if ((*m)->w == 0) {
        remove_monitor(*m);
        continue;
      }

      // Test if there are any clones in the set
      for (auto m2 = monitors.begin(); m2 != monitors.end(); m2++) {
        if ((*m) == (*m2) || (*m2)->w == 0) {
          continue;
        }

        // clang-format off
        if ((*m2)->x >= (*m)->x && (*m2)->x + (*m2)->w <= (*m)->x + (*m)->w &&
            (*m2)->y >= (*m)->y && (*m2)->y + (*m2)->h <= (*m)->y + (*m)->h) {
          // Reset width so that the output gets
          // removed in the base loop
          (*m2)->w = 0;
        }
        // clang-format on
      }
    }

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
    auto atom = Backlight;
    auto reply = conn.query_output_property(mon->output, atom);

    dst.min = 0;
    dst.max = 0;

    if (!reply->range || reply->length != 2) {
      atom = BACKLIGHT;
      reply = conn.query_output_property(mon->output, atom);
    }

    if (!reply->range || reply->length != 2) {
      return;
    }

    auto range = reply.valid_values().begin();
    dst.min = static_cast<uint32_t>(*range++);
    dst.max = static_cast<uint32_t>(*range);
    dst.atom = atom;
  }

  /**
   * Get backlight value for given output
   */
  void get_backlight_value(connection& conn, const monitor_t& mon, backlight_values& dst) {
    dst.val = 0;

    if (!dst.atom) {
      return;
    }

    auto reply = conn.get_output_property(mon->output, dst.atom, XCB_ATOM_NONE, 0, 4, 0, 0);

    if (reply->num_items == 1 && reply->format == 32 && reply->type == XCB_ATOM_INTEGER)
      dst.val = *reinterpret_cast<uint32_t*>(xcb_randr_get_output_property_data(reply.get().get()));
  }
}

POLYBAR_NS_END
