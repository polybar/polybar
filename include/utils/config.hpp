#pragma once

#include <mutex>

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

  std::string get_bar_path();
  void set_bar_path(const std::string& path);

  void load(const std::string& path);
  void load(const char *dir, const std::string& path);
  // void reload();

  boost::property_tree::ptree get_tree();

  std::string build_path(const std::string& section, const std::string& key);

  template<typename T>
  T dereference_var(const std::string& ref_section, const std::string& ref_key, const std::string& var, const T ref_val)
  {
    std::lock_guard<std::recursive_mutex> lck(config::mtx);

    std::string::size_type n, m;

    if ((n = var.find("${")) != 0) return ref_val;
    if ((m = var.find("}")) != var.length()-1) return ref_val;

    auto path = var.substr(2, m-2);
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

  template<typename T>
  T get(const std::string& section, const std::string& key)
  {
    std::lock_guard<std::recursive_mutex> lck(config::mtx);

    auto val = get_tree().get_optional<T>(build_path(section, key));

    if (val == boost::none)
      throw MissingValueException("Missing property \""+ key +"\" in section ["+ section +"]");

    auto str_val = get_tree().get<std::string>(build_path(section, key));

    return dereference_var<T>(section, key, str_val, val.get());
  }

  template<typename T>
  T get(const std::string& section, const std::string& key, T default_value)
  {
    std::lock_guard<std::recursive_mutex> lck(config::mtx);

    auto val = get_tree().get_optional<T>(build_path(section, key));
    auto str_val = get_tree().get_optional<std::string>(build_path(section, key));

    return dereference_var<T>(section, key, str_val.get_value_or(""), val.get_value_or(default_value));
  }

  template<typename T>
  std::vector<T> get_list(const std::string& section, const std::string& key)
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

  template<typename T>
  std::vector<T> get_list(const std::string& section, const std::string& key, std::vector<T> default_value)
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
