#pragma once

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "common.hpp"
#include "components/logger.hpp"
#include "components/x11/xresources.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

#define GET_CONFIG_VALUE(section, var, name) var = m_conf.get<decltype(var)>(section, name, var)
#define REQ_CONFIG_VALUE(section, var, name) var = m_conf.get<decltype(var)>(section, name)

using ptree = boost::property_tree::ptree;

DEFINE_ERROR(value_error);
DEFINE_ERROR(key_error);

class config {
 public:
  /**
   * Construct config
   */
  explicit config(const logger& logger, const xresource_manager& xrm)
      : m_logger(logger), m_xrm(xrm) {}

  /**
   * Load configuration and validate bar section
   *
   * This is done outside the constructor due to boost::di noexcept
   */
  void load(string file, string barname) {
    m_file = file;
    m_current_bar = barname;

    if (!file_util::exists(file))
      throw application_error("Could not find config file: " + file);

    try {
      boost::property_tree::read_ini(file, m_ptree);
    } catch (const std::exception& e) {
      throw application_error(e.what());
    }

    auto bars = defined_bars();
    if (std::find(bars.begin(), bars.end(), m_current_bar) == bars.end())
      throw application_error("Undefined bar: " + m_current_bar);

    if (has_env("XDG_CONFIG_HOME"))
      file = string_util::replace(file, read_env("XDG_CONFIG_HOME"), "$XDG_CONFIG_HOME");
    if (has_env("HOME"))
      file = string_util::replace(file, read_env("HOME"), "~");
    m_logger.trace("config: Loaded %s", file);
    m_logger.trace("config: Current bar section: [%s]", bar_section());
  }

  /**
   * Get path of loaded file
   */
  string filepath() const {
    return m_file;
  }

  /**
   * Get the section name of the bar in use
   */
  string bar_section() const {
    return "bar/" + m_current_bar;
  }

  /**
   * Get a list of defined bar sections in the current config
   */
  vector<string> defined_bars() const {
    vector<string> bars;

    for (auto&& p : m_ptree) {
      if (p.first.compare(0, 4, "bar/") == 0)
        bars.emplace_back(p.first.substr(4));
    }

    return bars;
  }

  /**
   * Build path used to find a parameter in the given section
   */
  string build_path(const string& section, const string& key) const {
    return section + "." + key;
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

    return dereference_var<T>(section, key, str_val, val.get());
  }

  /**
   * Get value of a variable by section and parameter name
   * with a default value in case the parameter isn't defined
   */
  template <typename T>
  T get(string section, string key, T default_value) const {
    auto val = m_ptree.get_optional<T>(build_path(section, key));
    auto str_val = m_ptree.get_optional<string>(build_path(section, key));

    return dereference_var<T>(
        section, key, str_val.get_value_or(""), val.get_value_or(default_value));
  }

  /**
   * Get list of values for the current bar by name
   */
  template <typename T>
  T get_list(string key) const {
    return get_list<T>(bar_section(), key);
  }

  /**
   * Get list of values by section and parameter name
   */
  template <typename T>
  vector<T> get_list(string section, string key) const {
    vector<T> vec;
    optional<T> value;

    while ((value = m_ptree.get_optional<T>(
                build_path(section, key) + "-" + to_string(vec.size()))) != boost::none) {
      auto str_val = m_ptree.get<string>(build_path(section, key) + "-" + to_string(vec.size()));
      vec.emplace_back(dereference_var<T>(section, key, str_val, value.get()));
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
    optional<T> value;

    while ((value = m_ptree.get_optional<T>(
                build_path(section, key) + "-" + to_string(vec.size()))) != boost::none) {
      auto str_val = m_ptree.get<string>(build_path(section, key) + "-" + to_string(vec.size()));
      vec.emplace_back(dereference_var<T>(section, key, str_val, value.get()));
    }

    if (vec.empty())
      return default_value;
    else
      return vec;
  }

 protected:
  /**
   * Find value of a config parameter defined as a reference
   * variable using ${section.param} or ${env:VAR}
   * ${self.key} may be used to reference the current bar section
   *
   * @deprecated: ${BAR.key} has been replaced with ${self.key}
   * but the former is kept to avoid breaking current configs
   */
  template <typename T>
  T dereference_var(string ref_section, string ref_key, string var, const T ref_val) const {
    auto n = var.find("${");
    auto m = var.find("}");

    if (n != 0 || m != var.length() - 1)
      return ref_val;

    auto path = var.substr(2, m - 2);

    if (path.find("env:") == 0) {
      if (has_env(path.substr(4).c_str()))
        return boost::lexical_cast<T>(read_env(path.substr(4).c_str()));
      return ref_val;
    }

    if (path.find("xrdb:") == 0) {
      if (std::is_same<string, T>::value)
        return boost::lexical_cast<T>(m_xrm.get_string(path.substr(5)));
      else if (std::is_same<float, T>::value)
        return boost::lexical_cast<T>(m_xrm.get_float(path.substr(5)));
      else if (std::is_same<int, T>::value)
        return boost::lexical_cast<T>(m_xrm.get_int(path.substr(5)));
      return ref_val;
    }

    auto ref_path = build_path(ref_section, ref_key);

    if ((n = path.find(".")) == string::npos)
      throw value_error("Invalid reference defined at [" + ref_path + "]");

    auto section = path.substr(0, n);

    section = string_util::replace(section, "BAR", bar_section());
    section = string_util::replace(section, "self", bar_section());

    auto key = path.substr(n + 1, path.length() - n - 1);
    auto val = m_ptree.get_optional<T>(build_path(section, key));

    if (val == boost::none)
      throw value_error("Unexisting reference defined at [" + ref_path + "]");

    auto str_val = m_ptree.get<string>(build_path(section, key));

    return dereference_var<T>(section, key, str_val, val.get());
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

LEMONBUDDY_NS_END
