#pragma once

#include <unordered_map>

#include "common.hpp"
#include "components/config_base.hpp"
#include "components/config_ini.hpp"
// #include "components/config_lua.hpp"
#include "components/logger.hpp"
#include "errors.hpp"
#include "settings.hpp"
// #include "utils/env.hpp"
// #include "utils/file.hpp"
// #include "utils/string.hpp"
// #if WITH_XRM
// #include "x11/xresources.hpp"
// #endif

POLYBAR_NS

class config {
 public:
  explicit config(const logger& logger, const string& path, const string& bar)
      : m_log(logger), m_file(path), m_barname(bar){};
  shared_ptr<config_ini> make_ini() {
    m_config_ini = make_shared<config_ini>(m_log, m_file, m_barname);
    return m_config_ini;
  }

  const string& filepath() const {
    return m_config_ini->filepath();
  }
  string section() const {
    return m_config_ini->section();
  }

  file_list get_included_files() const {
    return m_config_ini->get_included_files();
  }

  void warn_deprecated(const string& section, const string& key, string replacement="") const {
    return m_config_ini->warn_deprecated(section, key, replacement);
  }

  /**
   * Returns true if a given parameter exists
   */
  bool has(const string& section, const string& key) const {
    return m_config_ini->has(section, key);
  }

  /**
   * Get parameter for the current bar by name
   */
  template <typename T = string>
  T get(const string& key) const {
    return m_config_ini->get<T>(key);
  }

  /**
   * Get value of a variable by section and parameter name
   */
  template <typename T = string>
  T get(const string& section, const string& key) const {
    return m_config_ini->get<T>(section, key);
  }

  /**
   * Get value of a variable by section and parameter name
   * with a default value in case the parameter isn't defined
   */
  template <typename T = string>
  T get(const string& section, const string& key, const T& default_value) const {
    return m_config_ini->get<T>(section, key, default_value);
  }

  /**
   * Get list of key-value pairs starting with a prefix by section.
   *
   * Eg: if you have in config `env-FOO = bar`,
   *    get_with_prefix(section, "env-") will return [{"FOO", "bar"}]
   */
  template <typename T = string>
  vector<pair<string, T>> get_with_prefix(const string& section, const string& key_prefix) const {
    return m_config_ini->get_with_prefix<T>(section, key_prefix);
  }

  /**
   * Get list of values for the current bar by name
   */
  template <typename T = string>
  vector<T> get_list(const string& key) const {
    return m_config_ini->get_list<T>(key);
  }

  /**
   * Get list of values by section and parameter name
   */
  template <typename T = string>
  vector<T> get_list(const string& section, const string& key) const {
    return m_config_ini->get_list<T>(section, key);
  }

  /**
   * Get list of values by section and parameter name
   * with a default list in case the list isn't defined
   */
  template <typename T = string>
  vector<T> get_list(const string& section, const string& key, const vector<T>& default_value) const {
    return m_config_ini->get_list<T>(section, key, default_value);
  }

  /**
   * Attempt to load value using the deprecated key name. If successful show a
   * warning message. If it fails load the value using the new key and given
   * fallback value
   */
  template <typename T = string>
  T deprecated(const string& section, const string& old, const string& newkey, const T& fallback) const {
    return m_config_ini->deprecated<T>(section, old, newkey, fallback);
  }

 private:
  const logger& m_log;
  string m_file;
  string m_barname;

  shared_ptr<config_ini> m_config_ini;
};

POLYBAR_NS_END
