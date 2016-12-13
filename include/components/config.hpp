#pragma once

#include <boost/optional.hpp>
#include <unordered_map>

#include "common.hpp"
#include "components/logger.hpp"
#include "errors.hpp"
#include "utils/env.hpp"
#include "utils/string.hpp"
#include "x11/xresources.hpp"

POLYBAR_NS

using boost::optional;
using boost::none;

#define GET_CONFIG_VALUE(section, var, name) var = m_conf.get<decltype(var)>(section, name, var)
#define REQ_CONFIG_VALUE(section, var, name) var = m_conf.get<decltype(var)>(section, name)

DEFINE_ERROR(value_error);
DEFINE_ERROR(key_error);

class config {
 public:
  using valuemap_t = std::unordered_map<string, string>;
  using sectionmap_t = std::unordered_map<string, valuemap_t>;

  static constexpr const char* KEY_INHERIT{"inherit"};

  explicit config(const logger& logger, const xresource_manager& xrm) : m_logger(logger), m_xrm(xrm) {}

  void load(string file, string barname);
  string filepath() const;
  string bar_section() const;
  vector<string> defined_bars() const;
  void warn_deprecated(const string& section, const string& key, string replacement) const;

  /**
   * Returns true if a given parameter exists
   */
  template <typename T>
  bool has(const string& section, const string& key) const {
    auto it = m_sections.find(section);
    return it != m_sections.end() && it->second.find(key) != it->second.end();
  }

  /**
   * Get parameter for the current bar by name
   */
  template <typename T>
  T get(const string& key) const {
    return get<T>(bar_section(), key);
  }

  /**
   * Get value of a variable by section and parameter name
   */
  template <typename T>
  T get(const string& section, const string& key) const {
    optional<T> value{opt<T>(section, key)};

    if (value == none) {
      throw key_error("Missing parameter [" + section + "." + key + "]");
    }

    string string_value{opt<string>(section, key).get()};

    if (!string_value.empty()) {
      return dereference<T>(section, key, opt<string>(section, key).get(), value.get());
    }

    return move(value.get());
  }

  /**
   * Get value of a variable by section and parameter name
   * with a default value in case the parameter isn't defined
   */
  template <typename T>
  T get(const string& section, const string& key, const T& default_value) const {
    optional<T> value{opt<T>(section, key)};

    if (value == none) {
      return default_value;
    }

    string string_value{opt<string>(section, key).get()};

    if (!string_value.empty()) {
      return dereference<T>(section, key, opt<string>(section, key).get(), value.get());
    }

    return move(value.get());
  }

  /**
   * Get list of values for the current bar by name
   */
  template <typename T>
  vector<T> get_list(const string& key) const {
    return get_list<T>(bar_section(), key);
  }

  /**
   * Get list of values by section and parameter name
   */
  template <typename T>
  vector<T> get_list(const string& section, const string& key) const {
    vector<T> vec;
    optional<T> value;

    while ((value = opt<T>(section, key + "-" + to_string(vec.size()))) != none) {
      string string_value{get<string>(section, key + "-" + to_string(vec.size()))};

      if (!string_value.empty()) {
        vec.emplace_back(dereference<T>(section, key, move(string_value), move(value.get())));
      } else {
        vec.emplace_back(move(value.get()));
      }
    }

    if (vec.empty())
      throw key_error("Missing parameter [" + section + "." + key + "-0]");

    return vec;
  }

  /**
   * Get list of values by section and parameter name
   * with a default list in case the list isn't defined
   */
  template <typename T>
  vector<T> get_list(const string& section, const string& key, const vector<T>& default_value) const {
    vector<T> vec;
    optional<T> value;

    while ((value = opt<T>(section, key + "-" + to_string(vec.size()))) != none) {
      string string_value{get<string>(section, key + "-" + to_string(vec.size()))};

      if (!string_value.empty()) {
        vec.emplace_back(dereference<T>(section, key, move(string_value), move(value.get())));
      } else {
        vec.emplace_back(move(value.get()));
      }
    }

    if (vec.empty())
      return default_value;
    else
      return vec;
  }

 protected:
  void read();
  void copy_inherited();

  template <typename T>
  T convert(string&& value) const;

  template <typename T>
  const optional<T> opt(const string& section, const string& key) const {
    sectionmap_t::const_iterator s;
    valuemap_t::const_iterator v;

    if ((s = m_sections.find(section)) != m_sections.end() && (v = s->second.find(key)) != s->second.end()) {
      return {convert<T>(string{v->second})};
    }

    return none;
  }

  /**
   * Dereference value reference
   */
  template <typename T>
  T dereference(const string& section, const string& key, const string& var, T fallback) const {
    if (var.substr(0, 2) != "${" || var.substr(var.length() - 1) != "}") {
      return fallback;
    }

    auto path = var.substr(2, var.length() - 3);
    size_t pos;

    if (path.compare(0, 4, "env:") == 0) {
      return dereference_env<T>(path.substr(4), fallback);
    } else if (path.compare(0, 5, "xrdb:") == 0) {
      return dereference_xrdb<T>(path.substr(5), fallback);
    } else if ((pos = path.find(".")) != string::npos) {
      return dereference_local<T>(path.substr(0, pos), path.substr(pos + 1), section);
    } else {
      throw value_error("Invalid reference defined at [" + section + "." + key + "]");
    }
  }

  /**
   * Dereference local value reference defined using:
   *  ${root.key}
   *  ${self.key}
   *  ${section.key}
   */
  template <typename T>
  T dereference_local(string section, const string& key, const string& current_section) const {
    if (section == "BAR") {
      m_logger.warn("${BAR.key} is deprecated. Use ${root.key} instead");
    }

    section = string_util::replace(section, "BAR", bar_section(), 0, 3);
    section = string_util::replace(section, "root", bar_section(), 0, 4);
    section = string_util::replace(section, "self", current_section, 0, 4);

    optional<T> result{opt<T>(section, key)};

    if (result == none) {
      throw value_error("Unexisting reference defined [" + section + "." + key + "]");
    }

    return dereference<T>(section, key, opt<string>(section, key).get(), result.get());
  }

  /**
   * Dereference environment variable reference defined using:
   *  ${env:key}
   *  ${env:key:fallback value}
   */
  template <typename T>
  T dereference_env(string var, T fallback) const {
    size_t pos;

    if ((pos = var.find(":")) != string::npos) {
      fallback = convert<T>(var.substr(pos + 1));
      var.erase(pos);
    }

    if (env_util::has(var.c_str())) {
      return convert<T>(env_util::get(var.c_str()));
    } else {
      return fallback;
    }
  }

  /**
   * Dereference X resource db value defined using:
   *  ${xrdb:key}
   *  ${xrdb:key:fallback value}
   */
  template <typename T>
  T dereference_xrdb(string var, T fallback) const {
    size_t pos;

    if ((pos = var.find(":")) != string::npos) {
      return convert<T>(m_xrm.get_string(var.substr(0, pos), var.substr(pos + 1)));
    }

    string str{m_xrm.get_string(var, "")};
    return str.empty() ? fallback : convert<T>(move(str));
  }

 private:
  const logger& m_logger;
  const xresource_manager& m_xrm;
  string m_file;
  string m_current_bar;
  sectionmap_t m_sections;
};

namespace {
  /**
   * Configure injection module
   */
  template <typename T = const config&>
  di::injector<T> configure_config() {
    return di::make_injector(configure_logger(), configure_xresource_manager());
  }
}

POLYBAR_NS_END
