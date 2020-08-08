#pragma once

#include <map>

#include "settings.hpp"

#if not WITH_XKB
#error "X xkb extension is disabled..."
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wkeyword-macro"
#endif
#define explicit mask_cxx_explicit_keyword
#include <xcb/xkb.h>
#undef explicit
#include <xpp/proto/xkb.hpp>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "common.hpp"

POLYBAR_NS

using std::map;

// fwd
class connection;

namespace evt {
  using xkb_new_keyboard_notify = xpp::xkb::event::new_keyboard_notify<connection&>;
  using xkb_map_notify = xpp::xkb::event::map_notify<connection&>;
  using xkb_state_notify = xpp::xkb::event::state_notify<connection&>;
  using xkb_controls_notify = xpp::xkb::event::controls_notify<connection&>;
  using xkb_indicator_state_notify = xpp::xkb::event::indicator_state_notify<connection&>;
  using xkb_indicator_map_notify = xpp::xkb::event::indicator_map_notify<connection&>;
  using xkb_names_notify = xpp::xkb::event::names_notify<connection&>;
  using xkb_compat_map_notify = xpp::xkb::event::compat_map_notify<connection&>;
  using xkb_bell_notify = xpp::xkb::event::bell_notify<connection&>;
  using xkb_action_message = xpp::xkb::event::action_message<connection&>;
  using xkb_access_x_notify = xpp::xkb::event::access_x_notify<connection&>;
  using xkb_extension_device_notify = xpp::xkb::event::extension_device_notify<connection&>;
}  // namespace evt

class keyboard {
 public:
  struct indicator {
    enum class type { NONE = 0U, CAPS_LOCK, NUM_LOCK, SCROLL_LOCK };
    xcb_atom_t atom{};
    unsigned char mask{0};
    string name{};
    bool enabled{false};
  };

  struct layout {
    string group_name{};
    vector<string> symbols{};
  };

  explicit keyboard(vector<layout>&& layouts_, map<indicator::type, indicator>&& indicators_, unsigned char group)
      : layouts(forward<decltype(layouts)>(layouts_))
      , indicators(forward<decltype(indicators)>(indicators_))
      , current_group(group) {}

  const indicator& get(const indicator::type& i) const;
  void set(unsigned int state);
  bool on(const indicator::type&) const;
  void current(unsigned char group);
  unsigned char current() const;
  const string group_name(size_t index = 0) const;
  const string layout_name(size_t index = 0) const;
  const string variant_name(size_t index = 0) const;
  const string indicator_name(const indicator::type&) const;
  size_t size() const;

 private:
  vector<layout> layouts;
  map<indicator::type, indicator> indicators;
  unsigned char current_group{0};
};

namespace xkb_util {
  static constexpr const char* LAYOUT_SYMBOL_BLACKLIST{";group;inet;pc;"};

  void query_extension(connection& conn);

  void switch_layout(connection& conn, xcb_xkb_device_spec_t device, unsigned char index);
  unsigned char get_current_group(connection& conn, xcb_xkb_device_spec_t device);
  vector<keyboard::layout> get_layouts(connection& conn, xcb_xkb_device_spec_t device);
  map<keyboard::indicator::type, keyboard::indicator> get_indicators(connection& conn, xcb_xkb_device_spec_t device);
  string parse_layout_symbol(string&& name);
}  // namespace xkb_util

POLYBAR_NS_END
