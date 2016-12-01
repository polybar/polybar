#pragma once

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "common.hpp"
#include "components/logger.hpp"
#include "errors.hpp"
#include "utils/env.hpp"
#include "utils/string.hpp"
#include "x11/xresources.hpp"

POLYBAR_NS

#define GET_CONFIG_VALUE(section, var, name) var = m_conf.get<decltype(var)>(section, name, var)
#define REQ_CONFIG_VALUE(section, var, name) var = m_conf.get<decltype(var)>(section, name)

using ptree = boost::property_tree::ptree;

DEFINE_ERROR(value_error);
DEFINE_ERROR(key_error);

class config {
 public:
  static constexpr const char* KEY_INHERIT{"inherit"};

  explicit config(const logger& logger, const xresource_manager& xrm) : m_logger(logger), m_xrm(xrm) {}

  void load(string file, string barname);
  void copy_inherited();
  string filepath() const;
  string bar_section() const;
  vector<string> defined_bars() const;
  string build_path(const string& section, const string& key) const;
  void warn_deprecated(const string& section, const string& key, string replacement) const;

  /**
   * Returns true if a given parameter exists
   */
  template <typename T>
  bool has(string section, string key) const {
    auto val = m_ptree.get_optional<T>(build_path(section, key));
    return (val != boost::none);
  }

  /**
   * Get parameter for the current bar by name
   */
  template <typename T>
  T get(string key) const {
    return get<T>(bar_section(), key);
  }

  /**
   * Get value of a variable by section and parameter name
   */
  template <typename T>
  T get(string section, string key) const {
    auto val = m_ptree.get_optional<T>(build_path(section, key));

    if (val == boost::none)
      throw key_error("Missing parameter [" + section + "." + key + "]");

    auto str_val = m_ptree.get<string>(build_path(section, key));

    return dereference<T>(section, key, str_val, val.get());
  }

  /**
   * Get value of a variable by section and parameter name
   * with a default value in case the parameter isn't defined
   */
  template <typename T>
  T get(string section, string key, T default_value) const {
    auto val = m_ptree.get_optional<T>(build_path(section, key));
    auto str_val = m_ptree.get_optional<string>(build_path(section, key));

    return dereference<T>(section, key, str_val.get_value_or(""), val.get_value_or(default_value));
  }

  /**
   * Get list of values for the current bar by name
   */
  template <typename T>
  vector<T> get_list(string key) const {
    return get_list<T>(bar_section(), key);
  }

  /**
   * Get list of values by section and parameter name
   */
  template <typename T>
  vector<T> get_list(string section, string key) const {
    vector<T> vec;
    boost::optional<T> value;

    while ((value = m_ptree.get_optional<T>(build_path(section, key) + "-" + to_string(vec.size()))) != boost::none) {
      auto str_val = m_ptree.get<string>(build_path(section, key) + "-" + to_string(vec.size()));
      vec.emplace_back(dereference<T>(section, key, str_val, value.get()));
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
  vector<T> get_list(string section, string key, vector<T> default_value) const {
    vector<T> vec;
    boost::optional<T> value;

    while ((value = m_ptree.get_optional<T>(build_path(section, key) + "-" + to_string(vec.size()))) != boost::none) {
      auto str_val = m_ptree.get<string>(build_path(section, key) + "-" + to_string(vec.size()));
      vec.emplace_back(dereference<T>(section, key, str_val, value.get()));
    }

    if (vec.empty())
      return default_value;
    else
      return vec;
  }

 protected:
  /**
   * Dereference value reference
   */
  template <typename T>
  T dereference(string section, string key, string var, const T value) const {
    if (var.substr(0, 2) != "${" || var.substr(var.length() - 1) != "}") {
      return value;
    }

    auto path = var.substr(2, var.length() - 3);
    size_t pos;

    if (path.find("env:") == 0) {
      return dereference_env<T>(path.substr(4), value);
    } else if (path.find("xrdb:") == 0) {
      return dereference_xrdb<T>(path.substr(5), value);
    } else if ((pos = path.find(".")) != string::npos) {
      return dereference_local<T>(path.substr(0, pos), path.substr(pos + 1), section);
    } else {
      throw value_error("Invalid reference defined at [" + build_path(section, key) + "]");
    }
  }

  /**
   * Dereference local value reference defined using:
   *  ${root.key}
   *  ${self.key}
   *  ${section.key}
   */
  template <typename T>
  T dereference_local(string section, string key, string current_section) const {
    if (section == "BAR")
      m_logger.warn("${BAR.key} is deprecated. Use ${root.key} instead");

    section = string_util::replace(section, "BAR", bar_section(), 0, 3);
    section = string_util::replace(section, "root", bar_section(), 0, 4);
    section = string_util::replace(section, "self", current_section, 0, 4);

    auto path = build_path(section, key);
    auto result = m_ptree.get_optional<T>(path);

    if (result == boost::none)
      throw value_error("Unexisting reference defined [" + path + "]");

    return dereference<T>(section, key, m_ptree.get<string>(path), result.get());
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
      fallback = boost::lexical_cast<T>(var.substr(pos + 1));
      var.erase(pos);
    }

    if (env_util::has(var.c_str()))
      return boost::lexical_cast<T>(env_util::get(var.c_str()));

    return fallback;
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
      fallback = boost::lexical_cast<T>(var.substr(pos + 1));
      var.erase(pos);
    }

    if (std::is_same<string, T>::value)
      return boost::lexical_cast<T>(m_xrm.get_string(var, boost::lexical_cast<string>(fallback)));
    else if (std::is_same<float, T>::value)
      return boost::lexical_cast<T>(m_xrm.get_float(var, boost::lexical_cast<float>(fallback)));
    else if (std::is_same<int, T>::value)
      return boost::lexical_cast<T>(m_xrm.get_int(var, boost::lexical_cast<int>(fallback)));
    return fallback;
  }

 private:
  const logger& m_logger;
  const xresource_manager& m_xrm;
  ptree m_ptree;
  string m_file;
  string m_current_bar;
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
