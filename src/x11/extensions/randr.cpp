#include <algorithm>
#include <utility>

#include "components/types.hpp"
#include "errors.hpp"
#include "settings.hpp"
#include "utils/string.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/extensions/randr.hpp"

POLYBAR_NS

/**
 * Workaround for the inconsistent naming of outputs between intel and nvidia
 * drivers (xf86-video-intel drops the dash)
 *
 * If exact == false, dashes will be ignored while matching
 */
bool randr_output::match(const string& o, bool exact) const {
  if (exact) {
    return name == o;
  } else {
    return string_util::replace(name, "-", "") == string_util::replace(o, "-", "");
  }
}

/**
 * Match position
 */
bool randr_output::match(const position& p) const {
  return p.x == x && p.y == y;
}

namespace randr_util {
  /**
   * XRandR version
   */
  static unsigned int g_major_version = 0;
  static unsigned int g_minor_version = 0;

  /**
   * Query for the XRandR extension
   */
  void query_extension(connection& conn) {
    auto ext = conn.randr().query_version(XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);

    g_major_version = ext->major_version;
    g_minor_version = ext->minor_version;

    if (!conn.extension<xpp::randr::extension>()->present) {
      throw application_error("Missing X extension: Randr");
    }
  }

  /**
   * Check for XRandR monitor support
   */
  bool check_monitor_support() {
    return WITH_XRANDR_MONITORS && g_major_version >= 1 && g_minor_version >= 5;
  }

  /**
   * Define monitor
   */
  monitor_t make_monitor(
      xcb_randr_output_t randr, string name, unsigned short int w, unsigned short int h, short int x, short int y,
      bool primary) {
    monitor_t mon{new monitor_t::element_type{}};
    mon->output = randr;
    mon->name = move(name);
    mon->x = x;
    mon->y = y;
    mon->h = h;
    mon->w = w;
    mon->primary = primary;
    return mon;
  }

  /**
   * Create a list of all available randr outputs
   */
  vector<monitor_t> get_monitors(connection& conn, xcb_window_t root, bool connected_only, bool realloc) {
    static vector<monitor_t> monitors;

    if (realloc) {
      monitors.clear();
    } else if (!monitors.empty()) {
      return monitors;
    }

#if WITH_XRANDR_MONITORS
    if (check_monitor_support()) {
      for (auto&& mon : conn.get_monitors(root, true).monitors()) {
        try {
          auto name = conn.get_atom_name(mon.name).name();
          monitors.emplace_back(make_monitor(XCB_NONE, move(name), mon.width, mon.height, mon.x, mon.y, mon.primary));
        } catch (const exception&) {
          // silently ignore output
        }
      }
    }
#endif
    auto primary_output = conn.get_output_primary(root).output();
    string primary_name{};

    if (primary_output != XCB_NONE) {
      auto primary_info = conn.get_output_info(primary_output);
      auto name_iter = primary_info.name();
      primary_name = {name_iter.begin(), name_iter.end()};
    }

    for (auto&& output : conn.get_screen_resources(root).outputs()) {
      try {
        auto info = conn.get_output_info(output);
        if (info->crtc == XCB_NONE) {
          continue;
        } else if (connected_only && info->connection != XCB_RANDR_CONNECTION_CONNECTED) {
          continue;
        }

        auto name_iter = info.name();
        string name{name_iter.begin(), name_iter.end()};

#if WITH_XRANDR_MONITORS
        if (check_monitor_support()) {
          auto mon = std::find_if(
              monitors.begin(), monitors.end(), [&name](const monitor_t& mon) { return mon->name == name; });
          if (mon != monitors.end()) {
            (*mon)->output = output;
            continue;
          }
        }
#endif

        auto crtc = conn.get_crtc_info(info->crtc);
        auto primary = (primary_name == name);
        monitors.emplace_back(make_monitor(output, move(name), crtc->width, crtc->height, crtc->x, crtc->y, primary));
      } catch (const exception&) {
        // silently ignore output
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
      for (auto& monitor : monitors) {
        if ((*m) == monitor || monitor->w == 0) {
          continue;
        } else if (check_monitor_support() && (monitor->output == XCB_NONE || (*m)->output == XCB_NONE)) {
          continue;
        }

        // clang-format off
        if (monitor->x >= (*m)->x && monitor->x + monitor->w <= (*m)->x + (*m)->w &&
            monitor->y >= (*m)->y && monitor->y + monitor->h <= (*m)->y + (*m)->h) {
          // Reset width so that the output gets
          // removed in the base loop
          monitor->w = 0;
        }
        // clang-format on
      }
    }

    sort(monitors.begin(), monitors.end(), [](monitor_t& m1, monitor_t& m2) -> bool {
      if (m1->x < m2->x || m1->y + m1->h <= m2->y) {
        return true;
      }
      if (m1->x > m2->x || m1->y + m1->h > m2->y) {
        return -1;
      }
      return false;
    });

    return monitors;
  }

  /**
   * Searches for a monitor with name in monitors
   *
   * Does best-fit matching (if exact_match == true, this is also first-fit)
   */
  monitor_t match_monitor(vector<monitor_t> monitors, const string& name, bool exact_match) {
    monitor_t result{};
    for(auto&& monitor : monitors) {
      // If we can do an exact match, we have found our result
      if(monitor->match(name, true)) {
        result = move(monitor);
        break;
      }

      /*
       * Non-exact matches are moved into the result but we continue searching
       * through the list, maybe we can find an exact match
       * Note: If exact_match == true, we don't need to run this because it
       * would be the exact same check as above
       */
      if(!exact_match && monitor->match(name, false)) {
        result = move(monitor);
      }
    }

    return result;
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
    dst.min = static_cast<double>(*range++);
    dst.max = static_cast<double>(*range);
    dst.atom = atom;
  }

  /**
   * Get backlight value for given output
   */
  void get_backlight_value(connection& conn, const monitor_t& mon, backlight_values& dst) {
    dst.val = 0.0;

    if (!dst.atom) {
      return;
    }

    auto reply = conn.get_output_property(mon->output, dst.atom, XCB_ATOM_NONE, 0, 4, 0, 0);
    if (reply->num_items == 1 && reply->format == 32 && reply->type == XCB_ATOM_INTEGER) {
      int value = *reinterpret_cast<int*>(xcb_randr_get_output_property_data(reply.get().get()));
      dst.val = static_cast<double>(value);
    }
  }
}

POLYBAR_NS_END
