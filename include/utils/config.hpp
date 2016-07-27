#pragma once

#include <mutex>

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "exception.hpp"
#include "utils/string.hpp"

namespace config
{
  DefineBaseException(ConfigException);
  DefineChildException(UnexistingFileError, ConfigException);
  DefineChildException(ParseError, ConfigException);
  DefineChildException(MissingValueException, ConfigException);
  DefineChildException(MissingListValueException, ConfigException);
  DefineChildException(InvalidVariableException, ConfigException);
  DefineChildException(InvalidReferenceException, ConfigException);

  static std::recursive_mutex mtx;

  /**
   * Gets the path used to access the current bar section
   */
  std::string get_bar_path();

  /**
   * Sets the path used to access the current bar section
   */
  void set_bar_path(std::string path);

  /**
   * Loads a configuration file
   */
  void load(std::string path);

  /**
   * Loads a configuration file
   */
  void load(const char *dir, std::string path);

  /**
   * Reloads the configuration values
   * TODO: Implement properly
   */
  // void reload();

  /**
   * Gets the boost property tree handler
   */
  boost::property_tree::ptree get_tree();

  /**
   * Builds the path used to find a parameter in the given section
   */
  std::string build_path(std::string section, std::string key);

  /**
   * Gets the location of the configuration file
   */
  std::string get_file_path();

  /**
   * Finds the value of a config parameter defined
   * as a reference variable using ${section.param} or ${env:VAR}
   */
  template<typename T>
  T dereference_var(std::string ref_section, std::string ref_key, std::string var, const T ref_val)
  {
    std::lock_guard<std::recursive_mutex> lck(config::mtx);

    auto n = var.find("${");
    auto m = var.find("}");

    if (n != 0 || m != var.length()-1)
      return ref_val;

    auto path = var.substr(2, m-2);

    if (path.find("env:") == 0) {
      auto *envvar_value = std::getenv(path.substr(4).c_str());
      if (envvar_value != nullptr)
        return boost::lexical_cast<T>(envvar_value);
      return ref_val;
    }

    auto ref_path = build_path(ref_section, ref_key);

    if ((n = path.find(".")) == std::string::npos)
      throw InvalidVariableException("Invalid variable "+ ref_path +" => ${"+ path +"}");

    auto section = string::replace(path.substr(0, n), "BAR", get_bar_path());
    auto key = path.substr(n+1, path.length()-n-1);

    auto val = get_tree().get_optional<T>(build_path(section, key));

    if (val == boost::none)
      throw InvalidReferenceException("Variable defined at ["+ ref_path +"] points to a non existing parameter: ["+ build_path(section, key) +"]");

    auto str_val = get_tree().get<std::string>(build_path(section, key));

    return dereference_var<T>(section, key, str_val, val.get());
  }

  /**
   * Gets the value of a variable by section and parameter name
   */
  template<typename T>
  T get(std::string section, std::string key)
  {
    std::lock_guard<std::recursive_mutex> lck(config::mtx);

    auto val = get_tree().get_optional<T>(build_path(section, key));

    if (val == boost::none)
      throw MissingValueException("Missing property \""+ key +"\" in section ["+ section +"]");

    auto str_val = get_tree().get<std::string>(build_path(section, key));

    return dereference_var<T>(section, key, str_val, val.get());
  }

  /**
   * Gets the value of a variable by section and parameter name
   * with a default value in case the parameter isn't defined
   */
  template<typename T>
  T get(std::string section, std::string key, T default_value)
  {
    std::lock_guard<std::recursive_mutex> lck(config::mtx);

    auto val = get_tree().get_optional<T>(build_path(section, key));
    auto str_val = get_tree().get_optional<std::string>(build_path(section, key));

    return dereference_var<T>(section, key, str_val.get_value_or(""), val.get_value_or(default_value));
  }

  /**
   * Gets a list of values by section and parameter name
   */
  template<typename T>
  std::vector<T> get_list(std::string section, std::string key)
  {
    std::lock_guard<std::recursive_mutex> lck(config::mtx);

    std::vector<T> vec;
    boost::optional<T> value;

    while ((value = get_tree().get_optional<T>(build_path(section, key) + "-"+ std::to_string(vec.size()))) != boost::none) {
      auto str_val = get_tree().get<std::string>(build_path(section, key) + "-"+ std::to_string(vec.size()));
      vec.emplace_back(dereference_var<T>(section, key, str_val, value.get()));
    }

    if (vec.empty())
      throw MissingListValueException("Missing property \""+ key + "-0\" in section ["+ section +"]");

    return vec;
  }

  /**
   * Gets a list of values by section and parameter name
   * with a default list in case the list isn't defined
   */
  template<typename T>
  std::vector<T> get_list(std::string section, std::string key, std::vector<T> default_value)
  {
    std::lock_guard<std::recursive_mutex> lck(config::mtx);

    std::vector<T> vec;
    boost::optional<T> value;

    while ((value = get_tree().get_optional<T>(build_path(section, key) + "-"+ std::to_string(vec.size()))) != boost::none) {
      auto str_val = get_tree().get<std::string>(build_path(section, key) + "-"+ std::to_string(vec.size()));
      vec.emplace_back(dereference_var<T>(section, key, str_val, value.get()));
    }

    if (vec.empty())
      return default_value;
    else
      return vec;
  }
}
