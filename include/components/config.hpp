#pragma once

#include <unordered_map>

#include "common.hpp"
#include "components/logger.hpp"
#include "errors.hpp"
#include "settings.hpp"
#include "utils/env.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"
#if WITH_XRM
#include "x11/xresources.hpp"
#endif

POLYBAR_NS

DEFINE_ERROR(value_error);
DEFINE_ERROR(key_error);

using valuemap_t = std::unordered_map<string, string>;
using sectionmap_t = std::map<string, valuemap_t>;
using file_list = vector<string>;

class config {
 public:
  explicit config(const logger& logger, string&& path, string&& bar)
      : m_log(logger), m_file(move(path)), m_barname(move(bar)){};

  const string& filepath() const;
  const string& bar_name() const;

  static constexpr const char* BAR_PREFIX = "bar/";

  /**
   * @brief Instruct the config to connect to the xresource manager
   */
  void use_xrm();

  void set_sections(sectionmap_t sections);

  void set_included(file_list included);

  file_list get_included_files() const;

  /**
   * Returns true if a given parameter exists in the bar config
   */
  bool bar_has(const string& key) const {
    auto it = m_sections.find(section());
    return it != m_sections.end() && it->second.find(key) != it->second.end();
  }

  /**
   * Get bar parameter for the current bar by name
   */
  template <typename T = string>
  T bar_get(const string& key) const {
    return get<T>(section(), key);
  }

  /**
   * Get bar value of a variable by parameter name
   * with a default value in case the parameter isn't defined
   */
  template <typename T = string>
  T bar_get(const string& key, const T& default_value) const {
    return get<T>(section(), key, default_value);
  }

  /**
   * Attempt to load value using the deprecated key name. If successful show a
   * warning message. If it fails load the value using the new key and given
   * fallback value
   */
  template <typename T = string>
  T bar_deprecated(const string& old, const string& newkey, const T& fallback) const {
    try {
      T value{get<T>(section(), old)};
      warn_deprecated(section(), old, newkey);
      return value;
    } catch (const key_error& err) {
      return get<T>(section(), newkey, fallback);
    } catch (const std::exception& err) {
      m_log.err("Invalid value for \"%s.%s\", using fallback key \"%s.%s\" (reason: %s)", section(), old, section(), newkey,
          err.what());
      return get<T>(section(), newkey, fallback);
    }
  }
  template <typename T = string>
  vector<T> bar_get_list(const string& key) const {
    return get_list<T>(section(), key);
  }

  /**
   * Get list of values by section and parameter name
   * with a default list in case the list isn't defined
   */
  template <typename T = string>
  vector<T> bar_get_list(const string& key, const vector<T>& default_value) const {
    return get_list<T>(section(), key, default_value);
  }

  /**
   * Get setting with a default value in case the parameter isn't defined
   */
  template <typename T = string>
  T setting_get(const string& key, const T& default_value) const {
    return get<T>("settings", key, default_value);
  }

  void setting_warn_deprecated(const string& key, string replacement = "") const;

  void warn_deprecated(const string& section, const string& key, string replacement = "") const;

  /**
   * Returns true if a given parameter exists
   */
  bool has(const string& section, const string& key) const;

  /**
   * Set parameter value
   */
  void set(const string& section, const string& key, string&& value);

  /**
   * Get value of a variable by section and parameter name
   */
  template <typename T = string>
  T get(const string& section, const string& key) const {
    auto it = m_sections.find(section);
    if (it == m_sections.end()) {
      throw key_error("Missing section \"" + section + "\"");
    }
    if (it->second.find(key) == it->second.end()) {
      throw key_error("Missing parameter \"" + section + "." + key + "\"");
    }
    return convert<T>(dereference(section, key, it->second.at(key)));
  }

  /**
   * Get value of a variable by section and parameter name
   * with a default value in case the parameter isn't defined
   */
  template <typename T = string>
  T get(const string& section, const string& key, const T& default_value) const {
    try {
      string string_value{get<string>(section, key)};
      return convert<T>(dereference(move(section), move(key), move(string_value)));
    } catch (const key_error& err) {
      return default_value;
    } catch (const std::exception& err) {
      m_log.err("Invalid value for \"%s.%s\", using default value (reason: %s)", section, key, err.what());
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
    auto it = m_sections.find(section);
    if (it == m_sections.end()) {
      throw key_error("Missing section \"" + section + "\"");
    }

    vector<pair<string, T>> list;
    for (const auto& kv_pair : it->second) {
      const auto& key = kv_pair.first;

      if (key.substr(0, key_prefix.size()) == key_prefix) {
        const T& val = get<T>(section, key);
        list.emplace_back(key.substr(key_prefix.size()), val);
      }
    }

    return list;
  }

  /**
   * Get list of values by section and parameter name
   */
  template <typename T = string>
  vector<T> get_list(const string& section, const string& key) const {
    vector<T> results;

    while (true) {
      try {
        string string_value{get<string>(section, key + "-" + to_string(results.size()))};

        if (!string_value.empty()) {
          results.emplace_back(convert<T>(dereference(section, key, move(string_value))));
        } else {
          results.emplace_back(convert<T>(move(string_value)));
        }
      } catch (const key_error& err) {
        break;
      }
    }

    if (results.empty()) {
      throw key_error("Missing parameter \"" + section + "." + key + "-0\"");
    }

    return results;
  }

  /**
   * Get list of values by section and parameter name
   * with a default list in case the list isn't defined
   */
  template <typename T = string>
  vector<T> get_list(const string& section, const string& key, const vector<T>& default_value) const {
    vector<T> results;

    while (true) {
      try {
        string string_value{get<string>(section, key + "-" + to_string(results.size()))};

        if (!string_value.empty()) {
          results.emplace_back(convert<T>(dereference(section, key, move(string_value))));
        } else {
          results.emplace_back(convert<T>(move(string_value)));
        }
      } catch (const key_error& err) {
        break;
      } catch (const std::exception& err) {
        m_log.err("Invalid value in list \"%s.%s\", using list as-is (reason: %s)", section, key, err.what());
        return default_value;
      }
    }

    if (!results.empty()) {
      return results;
      ;
    }

    return default_value;
  }

 protected:
  void copy_inherited();

  template <typename T>
  T convert(string&& value) const;

  /**
   * Dereference value reference
   */
  string dereference(const string& section, const string& key, const string& var) const;

  /**
   * Dereference local value reference defined using:
   *  ${root.key}
   *  ${root.key:fallback}
   *  ${self.key}
   *  ${self.key:fallback}
   *  ${section.key}
   *  ${section.key:fallback}
   */
  string dereference_local(string section, const string& key, const string& current_section) const;

  /**
   * Dereference environment variable reference defined using:
   *  ${env:key}
   *  ${env:key:fallback value}
   */
  string dereference_env(string var) const;

  /**
   * Dereference X resource db value defined using:
   *  ${xrdb:key}
   *  ${xrdb:key:fallback value}
   */
  string dereference_xrdb(string var) const;

  /**
   * Dereference file reference by reading its contents
   *  ${file:/absolute/file/path}
   *  ${file:/absolute/file/path:fallback value}
   */
  string dereference_file(string var) const;

 public:
  string section() const;


  class value {
   public:
    enum class access_type {
      MAP,
      LIST
    };

    static constexpr const char* BARS_ENTRY = "bars";
    static constexpr const char* SETTINGS_ENTRY = "settings";
    static constexpr const char* MODULES_ENTRY = "modules";

    struct access_key {
      access_type access;
      unsigned int list_key;
      string map_key;
      access_key(const string& key): access(access_type::MAP), list_key(0), map_key(key) {}
      access_key(unsigned int key): access(access_type::LIST), list_key(key) {}
    };

    value(const config& conf, const std::string& key):m_conf(conf), m_keys({key}) {} 

    value operator[](const string &key) const {
      return value(*this, key);
    }

    value operator[](unsigned int key) const {
      return value(*this, key);
    }

    template <typename T>
    T as() const {
      // TODO check m_keys length >= 2
      access_key first = m_keys[0];
      if (first.access != access_type::MAP) {
          // TODO: error
      }
      if (first.map_key == BARS_ENTRY) {
        if (m_keys[1].access != access_type::MAP) {
          throw runtime_error("looking for bars as a list is not implemented yet");
        }
        if (m_keys[1].map_key != m_conf.bar_name()) {
          throw key_error("bar '" + m_keys[1].map_key + "' is not the current bar");
        }
          // TODO: need to check the type and the size of m_keys[1..  ]
        // TODO: assert m_keys[1].map_key == bar_name()
        return m_conf.bar_get<T>(m_keys[2].map_key);
      } else if (first.map_key == SETTINGS_ENTRY) {
        // return m_conf.setting_get<T>(m_keys[1].map_key);
        throw key_error("'settings' can only be accessed with a default value for now");
      } else if (first.map_key == MODULES_ENTRY) {
        throw runtime_error("not implemented");
        return T();
      }
      throw key_error("first key must be one of 'bars', 'settings' or 'modules', got '" + first.map_key + "'");
    }

    template <typename T>
    T as(const T& default_value) const {
      // TODO check m_keys length >= 2
      access_key first = m_keys[0];
      if (first.access != access_type::MAP) {
          // TODO: error
      }
      if (first.map_key == BARS_ENTRY) {
        if (m_keys[1].access != access_type::MAP) {
          throw runtime_error("looking for bars as a list is not implemented yet");
        }
        if (m_keys[1].map_key != m_conf.bar_name()) {
          throw key_error("bar '" + m_keys[1].map_key + "' is not the current bar");
        }
          // TODO: need to check the type and the size of m_keys[1..  ]
        // TODO: assert m_keys[1].map_key == bar_name()
        return m_conf.bar_get<T>(m_keys[2].map_key, default_value);
      } else if (first.map_key == SETTINGS_ENTRY) {
        return m_conf.setting_get<T>(m_keys[1].map_key, default_value);
      } else if (first.map_key == MODULES_ENTRY) {
        throw runtime_error("not implemented");
        return T();
      }
      throw key_error("first key must be one of 'bars', 'settings' or 'modules', got '" + first.map_key + "'");
    }

    template <typename T>
    vector<T> as_list() const {
      // TODO check m_keys length >= 2
      access_key first = m_keys[0];
      if (first.access != access_type::MAP) {
          // TODO: error
      }
      if (first.map_key == BARS_ENTRY) {
        if (m_keys[1].access != access_type::MAP) {
          throw runtime_error("looking for bars as a list is not implemented yet");
        }
        if (m_keys[1].map_key != m_conf.bar_name()) {
          throw key_error("bar '" + m_keys[1].map_key + "' is not the current bar");
        }
          // TODO: need to check the type and the size of m_keys[1..  ]
        // TODO: assert m_keys[1].map_key == bar_name()
        return m_conf.bar_get_list<T>(m_keys[2].map_key);
      } else if (first.map_key == SETTINGS_ENTRY) {
        throw value_error("Not a list");
      } else if (first.map_key == MODULES_ENTRY) {
        throw runtime_error("not implemented");
        return vector<T>();
      }
      throw key_error("first key must be one of 'bars', 'settings' or 'modules', got '" + first.map_key + "'");
    }

    template <typename T>
    T as_list(const vector<T>& default_value) const {
      // TODO check m_keys length >= 2
      access_key first = m_keys[0];
      if (first.access != access_type::MAP) {
          // TODO: error
      }
      if (first.map_key == BARS_ENTRY) {
        if (m_keys[1].access != access_type::MAP) {
          throw runtime_error("looking for bars as a list is not implemented yet");
        }
        if (m_keys[1].map_key != m_conf.bar_name()) {
          throw key_error("bar '" + m_keys[1].map_key + "' is not the current bar");
        }
          // TODO: need to check the type and the size of m_keys[1..  ]
        // TODO: assert m_keys[1].map_key == bar_name()
        return m_conf.bar_get<T>(m_keys[2].map_key, default_value);
      } else if (first.map_key == SETTINGS_ENTRY) {
        m_log.err("settings parameters are never lists");
        return default_value;
      } else if (first.map_key == MODULES_ENTRY) {
        throw runtime_error("not implemented");
        return default_value;
      }
      throw key_error("first key must be one of 'bars', 'settings' or 'modules', got '" + first.map_key + "'");
    }

  private:
    const config &m_conf;
    vector<access_key> m_keys;

    value(const value& parent, const std::string& key):m_conf(parent.m_conf), m_keys(parent.m_keys) {
      m_keys.push_back(access_key(key));
    } 
    value(const value& parent, unsigned int key):m_conf(parent.m_conf), m_keys(parent.m_keys) {
      m_keys.push_back(access_key(key));
    } 

  };
  value operator[](const string &key) const {
    value root(*this, key);
    return root;
  }

 private:
  const logger& m_log;
  string m_file;
  string m_barname;
  sectionmap_t m_sections{};

  /**
   * Absolute path of all files that were parsed in the process of parsing the
   * config (Path of the main config file also included)
   */
  file_list m_included;
#if WITH_XRM
  unique_ptr<xresource_manager> m_xrm;
#endif
};

POLYBAR_NS_END
