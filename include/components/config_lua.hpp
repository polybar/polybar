#pragma once

#include <unordered_map>
#include <map>
#include <sstream>

#include "common.hpp"
#include "components/config_base.hpp"
#include "components/types.hpp"
#include "components/logger.hpp"
#include "errors.hpp"
#include "settings.hpp"
#include "utils/color.hpp"
#include "utils/env.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"

#include <sol/sol.hpp>

POLYBAR_NS

class config_lua {
 public:
  explicit config_lua(const logger& logger, const string& path, const string& bar)
      : m_log(logger), m_file(path), m_barname(bar){
    m_state.open_libraries();

    m_state.script_file(path, [](lua_State*, sol::protected_function_result pfr) -> sol::protected_function_result {
  		sol::error err = pfr;
      throw application_error(string("Error loading lua config: ") + err.what(), 0);
    });

    if (m_barname.empty()) {
      sol::optional<sol::table> opt_bars = m_state[BARS_CONTAINER];
      if (opt_bars) {
        sol::table bars = *opt_bars;

        vector<string> keys(bars.size());
        for (const pair<sol::object, sol::object> &kv_pair: bars) {
          sol::object k = kv_pair.first;
          if (k.is<string>()) {
            keys.push_back(k.as<string>());
          } else {
            throw application_error("The config file contains a bar with a non string name.");
          }
        }

        if (keys.size() == 1) {
          m_barname = keys[0];
        } else if (bars.empty()){
          throw application_error("The config file contains no bar.");
        } else {
          throw application_error("The config file contains multiple bars, but no bar name was given. Available bars: " +
                                  string_util::join(keys, ", "));
        }
      } else {
        throw application_error("The config file contains no bar.");
      }
    }
  }

  const string& filepath() const;
  string section() const;

  static constexpr const char* BARS_CONTAINER = "bars";
  static constexpr const char* MODULES_CONTAINER = "modules";

  file_list get_included_files() const;

  void warn_deprecated(const string& section, const string& key, string replacement = "") const;

  /**
   * Returns true if a given parameter exists
   */
  bool has(const string& section, const string& key) const;

  /**
   * Get parameter for the current bar by name
   */
  template <typename T = string>
  T get(const string& key) const {
    return get<T>(section(), key);
  }

  /**
   * Get value of a variable by section and parameter name
   */
  template <typename T=string>
  T get(const string& section, const string& key) const {
    return config_utils::convert<T>(move(lua_get<string>(section, key)));
  }

  template <typename T=string>
  T get(const string& section, const string& key, const T& default_value) const {
    try {
      return get<T>(section, key);
    } catch (const key_error& err) {
      return default_value;
    } catch (const std::exception& err) {
      pair<string, std::optional<string>> entries = get_entries(section);
      if (entries.second){
        m_log.err("Invalid value for \"%s[%s][%s]\", using default value (reason: %s)", entries.first, *entries.second, key, err.what());
      } else {
        m_log.err("Invalid value for \"%s[%s]\", using default value (reason: %s)", entries.first, key, err.what());
      }

      return default_value;
    }
  }

  /**
   * Get list of key-value pairs starting with a prefix by section.
   *
   * Eg: if you have in config `env-FOO = bar`,
   *    get_with_prefix(section, "env-") will return [{"FOO", "bar"}]
   */
  template <typename T = string>
  vector<pair<string, T>> get_with_prefix(const string& section, const string& key_prefix) const {
    pair<sol::table, string> table_message = lua_get_table(section);
    sol::table table = table_message.first;
    vector<pair<string, T>> kv_vec;
    for (const pair<sol::object, sol::object> &kv_pair: table) {
      sol::object k = kv_pair.first;
      sol::object v = kv_pair.second;
      if (k.is<string>()) {
        string k_str = k.as<string>();
        if (k_str.substr(0, key_prefix.size()) == key_prefix) {
          if (v.is<T>()) {
            kv_vec.push_back({k_str, v.as<T>()});
          } else if (v.is<string>()) {
            kv_vec.push_back({k_str, config_utils::convert<T>(move(v.as<string>()))});
          } else {
            throw value_error("Wrong data type for \"" + table_message.second+ "[" + k_str + "]\"");
          }
        }
      }
    }
    return kv_vec;
  }

  /**
   * Get list of values for the current bar by name
   */
  template <typename T = string>
  vector<T> get_list(const string& key) const {
    return get_list<T>(section(), key);
  }

  /**
   * Get list of values by section and parameter name
   */
  template <typename T = string>
  vector<T> get_list(const string& section, const string& key) const {
    pair<sol::table, string> table_message = lua_get_table(section);
    sol::table table = table_message.first;
    string message_key = table_message.second;

    if (!config_lua::has(table, key)) {
      throw key_error("Missing parameter \"" + message_key + "[" + key + "]\"");
    }
    sol::optional<vector<sol::optional<string>>> value = table[key];
    if (value == sol::nullopt) {
      sol::optional<string> value = table[key];
      if (value != sol::nullopt) {
        throw key_error("Not a list value for parameter \"" + message_key + "[" + key + "]\"");
      } else {
        throw value_error("Wrong type for parameter \"" + message_key + "[" + key + "]\"");
      }
    }
    vector<sol::optional<string>> vec = *value;
    vector<T> result{vec.size()};
    for (size_t i = 0; i < vec.size(); i++) {
      const sol::optional<string>& value = vec[i];
      if (value) {
        string value_str = *value;
        result.push_back(config_utils::convert<T>(move(value_str)));
      } else {
        throw value_error("The element of \"" + message_key + "\" at position " + to_string(i) + " cannot be converted to the expected type");
      }
    }
    return result;
  }

  /**
   * Get list of values by section and parameter name
   * with a default list in case the list isn't defined
   */
  template <typename T = string>
  vector<T> get_list(const string& section, const string& key, const vector<T>& default_value) const {
    try {
      vector<T> results = get_list<T>(section, key);
      if (!results.empty()) {
        return results;
      }
      return default_value;

    } catch (const key_error&) {
      return default_value;
    } catch (const std::exception& err) {
      pair<string, std::optional<string>> entries = get_entries(section);
      if (entries.second){
        m_log.err("Invalid value for \"%s[%s][%s]\", using list as-is (reason: %s)", entries.first, *entries.second, key, err.what());
      } else {
        m_log.err("Invalid value for \"%s[%s]\", using list as-is (reason: %s)", entries.first, key, err.what());
      }
      return default_value;
    }
  }

  /**
   * Attempt to load value using the deprecated key name. If successful show a
   * warning message. If it fails load the value using the new key and given
   * fallback value
   */
  template <typename T = string>
  T deprecated(const string& section, const string& old, const string& newkey, const T& fallback) const {
    try {
      T value{get<T>(section, old)};
      warn_deprecated(section, old, newkey);
      return value;
    } catch (const key_error& err) {
      return get<T>(section, newkey, fallback);
    } catch (const std::exception& err) {
      // TODO improve message
      m_log.err("Invalid value for \"%s.%s\", using fallback key \"%s.%s\" (reason: %s)", section, old, section, newkey,
          err.what());
      pair<string, std::optional<string>> entries = get_entries(section);
      if (entries.second){
        m_log.err("Invalid value for \"%s[%s][%s]\", fallback key \"%s[%s][%s]\" (reason: %s)", entries.first, *entries.second, old, entries.first, *entries.second, newkey, err.what());
      } else {
        m_log.err("Invalid value for \"%s[%s]\", fallback key \"%s[%s]\"  (reason: %s)", entries.first, old, entries.first, newkey, err.what());
      }
      return get<T>(section, newkey, fallback);
    }
  }

 private:
  pair<string, std::optional<string>> get_entries(const string& section) const;

  /**
   * Returns true if a given parameter exists in the lua table
   */
  static bool has(const sol::table& table, const string& key);

  /**
   * Returns true if a given parameter exists in the lua table
   */
  static bool has(const sol::state& table, const string& key);

  pair<sol::table, string> lua_get_table(const string& section) const;

  template <typename T=string>
  T lua_get(const string& section, const string& key) const {
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

    if (!config_lua::has(table, key)) {
      throw key_error("Missing parameter \"" + message_key + "[" + key + "]\"");
    }
    sol::optional<T> value = table[key];
    if (value) {
      return *value;
    } else {
      throw value_error("Wrong type for parameter \"" + message_key + "[" + key + "]\"");
    }
  }

  const logger& m_log;
  string m_file;
  string m_barname;
	sol::state m_state;
};

POLYBAR_NS_END
