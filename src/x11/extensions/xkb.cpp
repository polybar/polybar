#include "x11/extensions/xkb.hpp"

#include "errors.hpp"
#include "utils/string.hpp"
#include "x11/connection.hpp"
#include "x11/extensions/all.hpp"

POLYBAR_NS

/**
 * Get next layout index
 */
const keyboard::indicator& keyboard::get(const indicator::type& i) const {
  return indicators.at(i);
}

/**
 * Update indicator states
 */
void keyboard::set(unsigned int state) {
  for (auto& i : indicators) {
    i.second.enabled = state & i.second.mask;
  }
}

/**
 * Get state for the given class
 */
bool keyboard::on(const indicator::type& i) const {
  return indicators.at(i).enabled;
}

/**
 * Set current group number
 */
void keyboard::current(unsigned char group) {
  current_group = group;
}

/**
 * Get current group number
 */
unsigned char keyboard::current() const {
  return current_group;
}

/**
 * Get current group name
 */
const string keyboard::group_name(size_t index) const {
  if (!layouts.empty() && index < layouts.size()) {
    return layouts[index].group_name;
  }
  return "";
}

/**
 * Get current layout name
 */
const string keyboard::layout_name(size_t index) const {
  if (index >= layouts.size() || index >= layouts[index].symbols.size()) {
    return "";
  }
  return layouts[index].symbols[index];
}

/**
 * Get indicator name
 */
const string keyboard::indicator_name(const indicator::type& i) const {
  return indicators.at(i).name;
}

size_t keyboard::size() const {
  return layouts.size();
}

namespace xkb_util {
  /**
   * Query for the XKB extension
   */
  void query_extension(connection& conn) {
    conn.xkb().use_extension(XCB_XKB_MAJOR_VERSION, XCB_XKB_MINOR_VERSION);

    if (!conn.extension<xpp::xkb::extension>()->present) {
      throw application_error("Missing X extension: XKb");
    }
  }

  /**
   * Get current group number
   */
  void switch_layout(connection& conn, xcb_xkb_device_spec_t device, unsigned char index) {
    xcb_xkb_latch_lock_state(conn, device, 0, 0, true, index, 0, 0, 0);
    xcb_flush(conn);
  }

  /**
   * Get current group number
   */
  unsigned char get_current_group(connection& conn, xcb_xkb_device_spec_t device) {
    unsigned char result{0};
    auto reply = xcb_xkb_get_state_reply(conn, xcb_xkb_get_state(conn, device), nullptr);
    if (reply != nullptr) {
      result = reply->group;
      free(reply);
    }
    return result;
  }

  /**
   * Get keyboard layouts
   */
  vector<keyboard::layout> get_layouts(connection& conn, xcb_xkb_device_spec_t device) {
    vector<keyboard::layout> results;

    unsigned int mask{XCB_XKB_NAME_DETAIL_GROUP_NAMES | XCB_XKB_NAME_DETAIL_SYMBOLS};
    auto reply = xcb_xkb_get_names_reply(conn, xcb_xkb_get_names(conn, device, mask), nullptr);

    if (reply == nullptr) {
      return results;
    }

    xcb_xkb_get_names_value_list_t values{};
    void* buffer = xcb_xkb_get_names_value_list(reply);
    xcb_xkb_get_names_value_list_unpack(buffer, reply->nTypes, reply->indicators, reply->virtualMods, reply->groupNames,
        reply->nKeys, reply->nKeyAliases, reply->nRadioGroups, reply->which, &values);

    using get_atom_name_reply = xpp::x::reply::checked::get_atom_name<connection&>;
    vector<get_atom_name_reply> replies;
    for (int i = 0; i < xcb_xkb_get_names_value_list_groups_length(reply, &values); i++) {
      replies.emplace_back(xpp::x::get_atom_name(conn, values.groups[i]));
    }

    for (const auto& reply : replies) {
      vector<string> sym_names;

      for (auto&& sym : string_util::split(conn.get_atom_name(values.symbolsName).name(), '+')) {
        if (!(sym = parse_layout_symbol(move(sym))).empty()) {
          sym_names.emplace_back(move(sym));
        }
      }

      results.emplace_back(keyboard::layout{static_cast<get_atom_name_reply>(reply).name(), sym_names});
    }

    free(reply);

    return results;
  }

  /**
   * Get keyboard indicators
   */
  map<keyboard::indicator::type, keyboard::indicator> get_indicators(connection& conn, xcb_xkb_device_spec_t device) {
    map<keyboard::indicator::type, keyboard::indicator> results;

    unsigned int mask{XCB_XKB_NAME_DETAIL_INDICATOR_NAMES};
    auto reply = xcb_xkb_get_names_reply(conn, xcb_xkb_get_names(conn, device, mask), nullptr);

    if (reply == nullptr) {
      return results;
    }

    xcb_xkb_get_names_value_list_t values{};
    void* buffer = xcb_xkb_get_names_value_list(reply);
    xcb_xkb_get_names_value_list_unpack(buffer, reply->nTypes, reply->indicators, reply->virtualMods, reply->groupNames,
        reply->nKeys, reply->nKeyAliases, reply->nRadioGroups, reply->which, &values);

    using get_atom_name_reply = xpp::x::reply::checked::get_atom_name<connection&>;
    map<xcb_atom_t, get_atom_name_reply> entries;
    for (int i = 0; i < xcb_xkb_get_names_value_list_indicator_names_length(reply, &values); i++) {
      entries.emplace(values.indicatorNames[i], xpp::x::get_atom_name(conn, values.indicatorNames[i]));
    }

    for (const auto& entry : entries) {
      auto name = static_cast<get_atom_name_reply>(entry.second).name();
      auto type = keyboard::indicator::type::NONE;

      if (string_util::compare(name, "caps lock")) {
        type = keyboard::indicator::type::CAPS_LOCK;
      } else if (string_util::compare(name, "num lock")) {
        type = keyboard::indicator::type::NUM_LOCK;
      } else if (string_util::compare(name, "scroll lock")) {
        type = keyboard::indicator::type::SCROLL_LOCK;
      } else {
        continue;
      }

      auto data = conn.xkb().get_named_indicator(device, 0, 0, entry.first);
      auto mask = (*conn.xkb().get_indicator_map(device, 1 << data->ndx).maps().begin()).mods;
      auto enabled = static_cast<bool>(data->on);

      results.emplace(type, keyboard::indicator{entry.first, mask, name, enabled});
    }

    free(reply);

    return results;
  }

  /**
   * Parse symbol name and exclude entries blacklisted entries
   */
  string parse_layout_symbol(string&& name) {
    size_t pos;
    while ((pos = name.find_first_of({'(', ':'})) != string::npos) {
      name.erase(pos);
    }
    if (string_util::contains(LAYOUT_SYMBOL_BLACKLIST, ";" + name + ";")) {
      return "";
    }
    return move(name);
  }
}

POLYBAR_NS_END
