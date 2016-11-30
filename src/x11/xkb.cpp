#include "x11/connection.hpp"

#include "errors.hpp"
#include "utils/string.hpp"
#include "x11/xkb.hpp"

POLYBAR_NS

/**
 * Get indicator name
 */
const keyboard::indicator& keyboard::get(const indicator::type& i) const {
  return indicators.at(i);
}

/**
 * Update indicator states
 */
void keyboard::set(uint32_t state) {
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
  if (!layouts.empty() && index < layouts.size() && !layouts[index].symbols.empty()) {
    return layouts[index].symbols[0];
  }
  return "";
}

/**
 * Get indicator name
 */
const string keyboard::indicator_name(const indicator::type& i) const {
  return indicators.at(i).name;
}

namespace xkb_util {
  /**
   * Get keyboard layouts
   */
  vector<keyboard::layout> get_layouts(connection& conn, xcb_xkb_device_spec_t device) {
    auto mask = XCB_XKB_NAME_DETAIL_GROUP_NAMES | XCB_XKB_NAME_DETAIL_SYMBOLS;
    auto reply = xcb_xkb_get_names_reply(conn, xcb_xkb_get_names(conn, device, mask), nullptr);
    xcb_xkb_get_names_value_list_t values;
    xcb_xkb_get_names_value_list_unpack(xcb_xkb_get_names_value_list(reply), reply->nTypes, reply->indicators,
        reply->virtualMods, reply->groupNames, reply->nKeys, reply->nKeyAliases, reply->nRadioGroups, reply->which,
        &values);
    auto len = xcb_xkb_get_names_value_list_groups_length(reply, &values);
    free(reply);

    vector<reply::get_atom_name> replies;
    for (int i = 0; i < len; i++) {
      replies.emplace_back(xpp::x::get_atom_name(conn, values.groups[i]));
    }

    vector<keyboard::layout> results;
    for (const auto& reply : replies) {
      vector<string> sym_names;

      for (auto&& sym : string_util::split(conn.get_atom_name(values.symbolsName).name(), '+')) {
        if (!(sym = parse_layout_symbol(move(sym))).empty()) {
          sym_names.emplace_back(move(sym));
        }
      }

      results.emplace_back(keyboard::layout{static_cast<reply::get_atom_name>(reply).name(), sym_names});
    }

    return results;
  }

  /**
   * Get keyboard indicators
   */
  map<keyboard::indicator::type, keyboard::indicator> get_indicators(connection& conn, xcb_xkb_device_spec_t device) {
    auto mask = XCB_XKB_NAME_DETAIL_INDICATOR_NAMES;
    auto reply = xcb_xkb_get_names_reply(conn, xcb_xkb_get_names(conn, device, mask), nullptr);
    xcb_xkb_get_names_value_list_t values;
    xcb_xkb_get_names_value_list_unpack(xcb_xkb_get_names_value_list(reply), reply->nTypes, reply->indicators,
        reply->virtualMods, reply->groupNames, reply->nKeys, reply->nKeyAliases, reply->nRadioGroups, reply->which,
        &values);
    auto len = xcb_xkb_get_names_value_list_indicator_names_length(reply, &values);
    free(reply);

    map<xcb_atom_t, reply::get_atom_name> entries;
    for (int i = 0; i < len; i++) {
      entries.emplace(values.indicatorNames[i], xpp::x::get_atom_name(conn, values.indicatorNames[i]));
    }

    map<keyboard::indicator::type, keyboard::indicator> results;
    for (const auto& entry : entries) {
      auto name = static_cast<reply::get_atom_name>(entry.second).name();
      auto type = keyboard::indicator::type::NONE;

      if (string_util::compare(name, "caps lock")) {
        type = keyboard::indicator::type::CAPS_LOCK;
      } else if (string_util::compare(name, "num lock")) {
        type = keyboard::indicator::type::NUM_LOCK;
      } else {
        continue;
      }

      auto data = conn.xkb().get_named_indicator(device, 0, 0, entry.first);
      auto mask = (*conn.xkb().get_indicator_map(device, 1 << data->ndx).maps().begin()).mods;
      auto enabled = static_cast<bool>(data->on);

      results.emplace(type, keyboard::indicator{entry.first, mask, name, enabled});
    }

    return results;
  }

  /**
   * Parse symbol name and exclude entries blacklisted entries
   */
  string parse_layout_symbol(string&& name) {
    auto pos = name.find('(');
    if (pos != string::npos) {
      name.erase(pos);
    }
    if (string_util::contains(LAYOUT_SYMBOL_BLACKLIST, ";" + name + ";")) {
      return "";
    }
    return name;
  }
}

POLYBAR_NS_END
