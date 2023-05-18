#include "x11/extensions/randr.hpp"

#include <algorithm>
#include <utility>

#include "components/types.hpp"
#include "errors.hpp"
#include "settings.hpp"
#include "utils/string.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"

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

/**
 * Checks if `this` contains the position p
 */
bool randr_output::contains(const position& p) const {
  return x <= p.x && y <= p.y && (x + w) > p.x && (y + h) > p.y;
}

/**
 * Checks if inner is contained within `this`
 *
 * Also returns true for outputs that occupy the exact same space
 */
bool randr_output::contains(const randr_output& inner) const {
  return inner.x >= x && inner.x + inner.w <= x + w && inner.y >= y && inner.y + inner.h <= y + h;
}

/**
 * Checks if the given output is the same as this
 *
 * Looks at xcb_randr_output_t, position, dimension, name and 'primary'
 */
bool randr_output::equals(const randr_output& o) const {
  return o.output == output && o.x == x && o.y == y && o.w == w && o.h == h && o.primary == primary && o.name == name;
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
  monitor_t make_monitor(xcb_randr_output_t randr, string name, unsigned short int w, unsigned short int h, short int x,
      short int y, bool primary) {
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
  vector<monitor_t> get_monitors(connection& conn, bool connected_only, bool purge_clones) {
    xcb_window_t root = conn.root();
    vector<monitor_t> monitors;
    bool found = false;

    if (check_monitor_support()) {
#if WITH_XRANDR_MONITORS
      /* Use C, because XPP does not provide access to output info from monitors. */
      xcb_generic_error_t* err;
      auto rrmonitors = xcb_randr_get_monitors_reply(conn, xcb_randr_get_monitors(conn, root, true), &err);
      if (err != NULL) {
        free(err);
      } else {
        for (auto iter = xcb_randr_get_monitors_monitors_iterator(rrmonitors); iter.rem;
             xcb_randr_monitor_info_next(&iter)) {
          auto mon = iter.data;
          auto name = conn.get_atom_name(mon->name).name();

          /* Output is only useful for some plugins. We take the first one. */
          int randr_output_len = xcb_randr_monitor_info_outputs_length(mon);
          auto randr_outputs = xcb_randr_monitor_info_outputs(mon);
          auto output = (randr_output_len >= 1) ? randr_outputs[0] : XCB_NONE;
          monitors.emplace_back(
              make_monitor(output, move(name), mon->width, mon->height, mon->x, mon->y, mon->primary));
          found = true;
        }
        free(rrmonitors);
      }
#endif
    }
    if (!found || !purge_clones) {
      auto primary_output = conn.get_output_primary(root).output();
      string primary_name{};

      if (primary_output != XCB_NONE) {
        auto primary_info = conn.get_output_info(primary_output);
        auto name_iter = primary_info.name();
        primary_name = {name_iter.begin(), name_iter.end()};
      }

      for (auto&& output : conn.get_screen_resources_current(root).outputs()) {
        try {
          auto info = conn.get_output_info(output);
          if (info->crtc == XCB_NONE) {
            continue;
          } else if (connected_only && info->connection != XCB_RANDR_CONNECTION_CONNECTED) {
            continue;
          }

          auto name_iter = info.name();
          string name{name_iter.begin(), name_iter.end()};

          auto crtc = conn.get_crtc_info(info->crtc);
          auto primary = (primary_name == name);
          if (!std::any_of(
                  monitors.begin(), monitors.end(), [name](const auto& monitor) { return monitor->name == name; })) {
            monitors.emplace_back(
                make_monitor(output, move(name), crtc->width, crtc->height, crtc->x, crtc->y, primary));
          }
        } catch (const exception&) {
          // silently ignore output
        }
      }
    }

    if (purge_clones) {
      for (auto& outer : monitors) {
        if (outer->w == 0) {
          continue;
        }

        for (auto& inner : monitors) {
          if (outer == inner || inner->w == 0) {
            continue;
          }

          // If inner is contained in outer, inner is removed
          // If both happen to be the same size and have the same coordinates,
          // inner is still removed but it doesn't matter since both occupy the
          // exact same space
          if (outer->contains(*inner)) {
            // Reset width so that the output gets removed afterwards
            inner->w = 0;
          }
        }
      }

      // Remove all cloned monitors (monitors with 0 width)
      monitors.erase(
          std::remove_if(monitors.begin(), monitors.end(), [](const auto& monitor) { return monitor->w == 0; }),
          monitors.end());
    }

    return monitors;
  }

  /**
   * Searches for a monitor with name in monitors
   *
   * Does best-fit matching (if exact_match == true, this is also first-fit)
   */
  monitor_t match_monitor(vector<monitor_t> monitors, const string& name, bool exact_match) {
    monitor_t result{};
    for (auto&& monitor : monitors) {
      // If we can do an exact match, we have found our result
      if (monitor->match(name, true)) {
        result = move(monitor);
        break;
      }

      /*
       * Non-exact matches are moved into the result but we continue searching
       * through the list, maybe we can find an exact match
       * Note: If exact_match == true, we don't need to run this because it
       * would be the exact same check as above
       */
      if (!exact_match && monitor->match(name, false)) {
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
} // namespace randr_util

POLYBAR_NS_END
