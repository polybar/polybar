#pragma once

#include "settings.hpp"

#if not WITH_XRANDR
#error "X RandR extension is disabled..."
#endif

#include <xcb/randr.h>

#include <xpp/proto/randr.hpp>

#include "common.hpp"

POLYBAR_NS

class connection;
struct position;

namespace evt {
  using randr_notify = xpp::randr::event::notify<connection&>;
  using randr_screen_change_notify = xpp::randr::event::screen_change_notify<connection&>;
} // namespace evt

struct backlight_values {
  unsigned int atom{0};
  double min{0.0};
  double max{0.0};
  double val{0.0};
};

struct randr_output {
  string name;
  unsigned short int w{0U};
  unsigned short int h{0U};
  short int x{0};
  short int y{0};
  xcb_randr_output_t output;
  backlight_values backlight;
  bool primary{false};

  bool match(const string& o, bool exact = true) const;
  bool match(const position& p) const;

  bool contains(const position& p) const;
  bool contains(const randr_output& output) const;

  bool equals(const randr_output& output) const;
};

using monitor_t = shared_ptr<randr_output>;

namespace randr_util {
  void query_extension(connection& conn);

  bool check_monitor_support();

  monitor_t make_monitor(xcb_randr_output_t randr, string name, unsigned short int w, unsigned short int h, short int x,
      short int y, bool primary);
  vector<monitor_t> get_monitors(connection& conn, bool connected_only = false, bool purge_clones = true);
  monitor_t match_monitor(vector<monitor_t> monitors, const string& name, bool exact_match);

  void get_backlight_range(connection& conn, const monitor_t& mon, backlight_values& dst);
  void get_backlight_value(connection& conn, const monitor_t& mon, backlight_values& dst);
} // namespace randr_util

POLYBAR_NS_END
