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
    static constexpr const char* BARS_KEY = "bar";
    static constexpr const char* SETTINGS_ENTRY = "settings";
    static constexpr const char* MODULES_ENTRY = "modules";
    static constexpr const char* MODULES_KEY = "module";

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

    bool has(const string& name) const {
      string section;
      string key;

      access_key first = m_keys[0];
      if (first.access != access_type::MAP) {
        throw key_error(sstream() << "first key must be one of '" << BARS_ENTRY << "', '" << SETTINGS_ENTRY << "' or '" << MODULES_ENTRY << "', got '" << first.list_key << "'");
      }
      if (m_keys.size() > 1 && m_keys[1].access != access_type::MAP) {
        throw runtime_error(sstream() << "listing '" << BARS_ENTRY << "', '" << SETTINGS_ENTRY << "' or '" << MODULES_ENTRY << "' is not a valid access");
      }
      if (first.map_key == BARS_ENTRY) {
        if (m_keys.size() == 1) {
          return m_conf.m_sections.find(sstream() << BARS_KEY << '/' << name) != m_conf.m_sections.end();
        }
        section = sstream() << BARS_KEY << '/' << m_keys[1].map_key;
        key = (*this)[name].build_key(2);
      } else if (first.map_key == MODULES_ENTRY) {
        if (m_keys.size() == 1) {
          return m_conf.m_sections.find(sstream() << MODULES_KEY << '/' << name) != m_conf.m_sections.end();
        }
        section = sstream() << MODULES_KEY << '/' << m_keys[1].map_key;
        key = (*this)[name].build_key(2);
      } else if (first.map_key == SETTINGS_ENTRY) {
        section = SETTINGS_ENTRY;
        key = (*this)[name].build_key(1);
      } else {
        // any access other than 'bars', 'settings' and 'modules' is an error
        throw key_error(sstream() << "first key must be one of '" << BARS_ENTRY << "', '" << SETTINGS_ENTRY << "' or '" << MODULES_ENTRY << "', got '" << first.map_key << "'");
      }

      auto it = m_conf.m_sections.find(section);
      if (it == m_conf.m_sections.end()) {
        throw key_error("Missing section \"" + section + "\"");
      }

      auto has_key = [&key](const auto& kv) { return kv.first == key || string_util::starts_with(kv.first, key + "-"); };
      const auto &section_entry = m_conf.m_sections.at(section);
      return std::find_if(section_entry.begin(), section_entry.end(), has_key) != section_entry.end();
    }

    template <typename T>
    T as() const {
      check_path();
      access_key first = m_keys[0];
      if (first.map_key == BARS_ENTRY) {
        return m_conf.bar_get<T>(build_key(2));
      } else if (first.map_key == SETTINGS_ENTRY) {
        throw key_error("'settings' can only be accessed with a default value for now");
      } else if (first.map_key == MODULES_ENTRY) {
        return m_conf.get<T>(sstream() << MODULES_KEY << "/" << m_keys[1].map_key, build_key(2));
      }
      // The case where the first key is neither BARS_ENTRY, SETTINGS_ENTRY or MODULES_ENTRY is handled
      // by check_path() that throws
      throw runtime_error("This statement should never be reached");
    }

    template <typename T>
    T as(const T& default_value) const {
      check_path();
      access_key first = m_keys[0];
      if (first.map_key == BARS_ENTRY) {
        return m_conf.bar_get<T>(build_key(2), default_value);
      } else if (first.map_key == SETTINGS_ENTRY) {
        return m_conf.setting_get<T>(m_keys[1].map_key, default_value);
      } else if (first.map_key == MODULES_ENTRY) {
        return m_conf.get<T>(sstream() << MODULES_KEY << "/" << m_keys[1].map_key, build_key(2), default_value);
      }
      // The case where the first key is neither BARS_ENTRY, SETTINGS_ENTRY or MODULES_ENTRY is handled
      // by check_path() that throws
      throw runtime_error("This statement should never be reached");
    }

    template <typename T>
    vector<T> as_list() const {
      check_path();
      access_key first = m_keys[0];
      if (first.map_key == BARS_ENTRY) {
        return m_conf.bar_get_list<T>(m_keys[2].map_key);
      } else if (first.map_key == SETTINGS_ENTRY) {
        throw value_error("settings parameters are never lists");
      } else if (first.map_key == MODULES_ENTRY) {
        return m_conf.get_list<T>(sstream() << MODULES_KEY << "/" << m_keys[1].map_key, build_key(2));
      }
      // The case where the first key is neither BARS_ENTRY, SETTINGS_ENTRY or MODULES_ENTRY is handled
      // by check_path() that throws
      throw runtime_error("This statement should never be reached");
    }

    template <typename T>
    vector<T> as_list(const vector<T>& default_value) const {
      check_path();
      access_key first = m_keys[0];
      if (first.map_key == BARS_ENTRY) {
        return m_conf.bar_get_list<T>(m_keys[2].map_key, default_value);
      } else if (first.map_key == SETTINGS_ENTRY) {
        return default_value;
      } else if (first.map_key == MODULES_ENTRY) {
        return m_conf.get_list<T>(sstream() << MODULES_KEY << "/" << m_keys[1].map_key, build_key(2), default_value);
        throw runtime_error("not implemented");
        return default_value;
      }
      // The case where the first key is neither BARS_ENTRY, SETTINGS_ENTRY or MODULES_ENTRY is handled
      // by check_path() that throws
      throw runtime_error("This statement should never be reached");
    }

    size_t size() const {
      string section;
      string key;
      
      // for now, we need at least two keys to access config
      if (m_keys.size() < 2) {
        // XXX: this if might be removed when more access types will be allowed
        throw key_error("there must be at least two keys to access the config");
      }
      access_key first = m_keys[0];
      if (first.access != access_type::MAP) {
        throw key_error(sstream() << "first key must be one of '" << BARS_ENTRY << "', '" << SETTINGS_ENTRY << "' or '" << MODULES_ENTRY << "', got '" << first.list_key << "'");
      }
      if (first.map_key == BARS_ENTRY) {
        // bars access, we ensure that 
        //   - the second key is a string and equals to the bar_name
        //   - there is at least a third key to select a entry in the bar config
        if (m_keys[1].access != access_type::MAP) {
          throw runtime_error("listing bars is not implemented yet");
        }
        if (m_keys[1].map_key != m_conf.bar_name()) {
          throw key_error(sstream() << "bar '" << m_keys[1].map_key << "' is not the current bar");
        }
        section = m_conf.section();
        key = build_key(2);
      } else if (first.map_key == MODULES_ENTRY) {
        // modules access, we ensure that the second key is a string
        if (m_keys[1].access != access_type::MAP) {
          throw runtime_error(sstream() << "listing '" << MODULES_ENTRY << "' is not implemented yet");
        }
        section = sstream() << MODULES_KEY << '/' << m_keys[1].map_key;
        key = build_key(2);
      } else if (first.map_key == SETTINGS_ENTRY) {
        // settings access, we ensure that the second key is a string
        if (m_keys[1].access != access_type::MAP) {
          throw runtime_error(sstream() << "listing '" << SETTINGS_ENTRY << "' is not implemented yet");
        }
        section = SETTINGS_ENTRY;
        key = build_key(1);
      } else {
        // any access other than 'bars', 'settings' and 'modules' is an error
        throw key_error(sstream() << "first key must be one of '" << BARS_ENTRY << "', '" << SETTINGS_ENTRY << "' or '" << MODULES_ENTRY << "', got '" << first.map_key << "'");
      }

      auto it = m_conf.m_sections.find(section);
      if (it == m_conf.m_sections.end()) {
        throw key_error("Missing section \"" + section + "\"");
      }

      size_t index = 0;
      bool found = true;
      while (found) {
        found = false;
        string key_prefix = sstream() << key << "-" << index;
        printf("looking for %s\n", key_prefix.c_str());
        for (const auto& kv_pair : it->second) {
          const auto& curr_key = kv_pair.first;

          if (curr_key.substr(0, key_prefix.size()) == key_prefix) {
            printf("  found key %s\n", curr_key.c_str());
            index++;
            found = true;
            break;
          }
        }
      }
      return index;
    }
    operator string() const {
       return build_key(0);
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

    void check_path() const {
      // for now, we need at least two keys to access config
      if (m_keys.size() < 2) {
        // XXX: this might be removed when more access types will be allowed
        throw key_error("there must be at least two keys to access the config");
      }
      access_key first = m_keys[0];
      if (first.access != access_type::MAP) {
        throw key_error(sstream() << "first key must be one of '" << BARS_ENTRY << "', '" << SETTINGS_ENTRY << "' or '" << MODULES_ENTRY << "', got '" << first.list_key << "'");
      }
      if (first.map_key == BARS_ENTRY) {
        // bars access, we ensure that 
        //   - the second key is a string and equals to the bar_name
        //   - there is at least a third key to select a entry in the bar config
        if (m_keys[1].access != access_type::MAP) {
          throw runtime_error("listing bars is not implemented yet");
        }
        if (m_keys[1].map_key != m_conf.bar_name()) {
          throw key_error(sstream() << "bar '" << m_keys[1].map_key << "' is not the current bar");
        }
        if (m_keys.size() < 3) {
          throw key_error("there must be a key access after the bar name");
        }
      } else if (first.map_key == MODULES_ENTRY) {
        // modules access, we ensure that the second key is a string
        if (m_keys[1].access != access_type::MAP) {
          throw runtime_error(sstream() << "listing '" << MODULES_ENTRY << "' is not implemented yet");
        }        
      } else if (first.map_key == SETTINGS_ENTRY) {
        // settings access, we ensure that the second key is a string
        if (m_keys[1].access != access_type::MAP) {
          throw runtime_error(sstream() << "listing '" << SETTINGS_ENTRY << "' is not implemented yet");
        }        
      } else {
        // any access other than 'bars', 'settings' and 'modules' is an error
        throw key_error(sstream() << "first key must be one of '" << BARS_ENTRY << "', '" << SETTINGS_ENTRY << "' or '" << MODULES_ENTRY << "', got '" << first.map_key << "'");
      }
    }

    string build_key(size_t first_index) const {
      sstream ss = sstream();
      for (size_t i = first_index; i < m_keys.size(); i++) {
        if (i != first_index) {
          ss << '-';
        }
        if (m_keys[i].access == access_type::MAP) {
          ss << m_keys[i].map_key;
        } else {
          ss << m_keys[i].list_key;
        }
      }
      return ss;
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
