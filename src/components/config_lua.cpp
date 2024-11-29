#include "components/config_lua.hpp"
#include "utils/units.hpp"

POLYBAR_NS

namespace chrono = std::chrono;

/**
 * Get path of loaded file
 */
const string& config_lua::filepath() const {
  return m_file;
}

/**
 * Get the section name of the bar in use
 */
string config_lua::section() const {
  return m_barname;
}

/**
 * Print a deprecation warning if the given parameter is set
 */
void config_lua::warn_deprecated(const string& section, const string& key, string replacement) const {
  if (has(section, key)) {
    if (replacement.empty()) {
      m_log.warn(
          "The config parameter '%s.%s' is deprecated, it will be removed in the future. Please remove it from your "
          "config",
          section, key);
    } else {
      m_log.warn(
          "The config parameter `%s.%s` is deprecated, use `%s.%s` instead.", section, key, section, move(replacement));
    }
  }
}

bool config_lua::has(const string& section, const string& key) const {
  try {
    pair<sol::table, string> table_message = lua_get_table(section);
    return config_lua::has(table_message.first, key);
  } catch (const key_error&) {
    return false;
  }
}

file_list config_lua::get_included_files() const {
  file_list fl{m_file};
  return fl;
}

template <>
bool config_lua::get(const string& section, const string& key) const {
  return lua_get<bool>(section, key);
}

template <>
double config_lua::get(const string& section, const string& key) const {
  return lua_get<double>(section, key);
}

template <>
size_t config_lua::get(const string& section, const string& key) const {
    return lua_get<size_t>(section, key);
}

template <>
int config_lua::get(const string& section, const string& key) const {
  return lua_get<int>(section, key);
}

pair<string, std::optional<string>> config_lua::get_entries(const string& section) const {
  if (section == m_barname) {
    return {BARS_CONTAINER, section};
  } else if (section == "settings") {
    return {section, std::nullopt};
  } else {
    // split section which should look like "module/xyz" into "modules" and "xyz"
    return {MODULES_CONTAINER, section.substr(strlen("module/"))};
  }
}

/**
 * Returns true if a given parameter exists in the lua table
 */
bool config_lua::has(const sol::table& table, const string& key) {
  for (const pair<sol::object, sol::object> &p: table) {
    sol::object k = p.first;
    if (k.is<string>() && k.as<string>() == key) {
        return true;
    }
  }
  return false;
}

/**
 * Returns true if a given parameter exists in the lua table
 */
bool config_lua::has(const sol::state& table, const string& key) {
  for (const pair<sol::object, sol::object> &p: table) {
    sol::object k = p.first;
    if (k.is<string>() && k.as<string>() == key) {
        return true;
    }
  }
  return false;
}

pair<sol::table, string> config_lua::lua_get_table(const string& section) const {
  pair<string, std::optional<string>> entries = get_entries(section);

  if (!config_lua::has(m_state, entries.first)) {
      throw key_error("Missing entry \"" + entries.first + "\"");
  }
  sol::optional<sol::table> opt_table = m_state[entries.first];
  if (opt_table == sol::nullopt) {
      throw key_error("Wrong type for entry \"" + entries.first + "\": expecting table");
  }
  string message_key = entries.first;
  sol::table table = *opt_table;
  if (entries.second) {
    message_key += "[" + *entries.second + "]";
    if (!config_lua::has(table, *entries.second)) {
        throw key_error("Missing entry \"" + message_key + "\"");
    }
    sol::optional<sol::table> opt_table = table[*entries.second];
    if (opt_table == sol::nullopt) {
        throw key_error("Wrong type for entry \"" + message_key + "\": expecting table");
    }
    table = *opt_table;
  }
  return {table, message_key};
}

POLYBAR_NS_END
