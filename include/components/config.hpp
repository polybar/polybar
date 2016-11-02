#pragma once

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "common.hpp"
#include "components/logger.hpp"
#include "utils/string.hpp"
#include "x11/xresources.hpp"

LEMONBUDDY_NS

#define GET_CONFIG_VALUE(section, var, name) var = m_conf.get<decltype(var)>(section, name, var)
#define REQ_CONFIG_VALUE(section, var, name) var = m_conf.get<decltype(var)>(section, name)

using ptree = boost::property_tree::ptree;

DEFINE_ERROR(value_error);
DEFINE_ERROR(key_error);

class config {
 public:
  explicit config(const logger& logger, const xresource_manager& xrm)
      : m_logger(logger), m_xrm(xrm) {}

  void load(string file, string barname);
  string filepath() const;
  string bar_section() const;
  vector<string> defined_bars() const;
  string build_path(const string& section, const string& key) const;

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
   * variable using ${section.param}
   *
   * ${BAR.key} may be used to reference the current bar section
   * ${self.key} may be used to reference the current section
   * ${env:key} may be used to reference an environment variable
   * ${xrdb:key} may be used to reference a variable in the X resource db
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
    section = string_util::replace(section, "self", ref_section);

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
