#include "components/config.hpp"

#include <climits>
#include <cmath>
#include <fstream>

#include "cairo/utils.hpp"
#include "components/types.hpp"
#include "utils/color.hpp"
#include "utils/env.hpp"
#include "utils/factory.hpp"
#include "utils/string.hpp"
#include "utils/units.hpp"

POLYBAR_NS

namespace chrono = std::chrono;

/**
 * Get path of loaded file
 */
const string& config::filepath() const {
  return m_file;
}

/**
 * Get the section name of the bar in use
 */
string config::section() const {
  return BAR_PREFIX + m_barname;
}

void config::use_xrm() {
#if WITH_XRM
  /*
   * Initialize the xresource manager if there are any xrdb refs
   * present in the configuration
   */
  if (!m_xrm) {
    m_log.info("Enabling xresource manager");
    m_xrm.reset(new xresource_manager{connection::make()});
  }
#endif
}

void config::set_sections(sectionmap_t sections) {
  m_sections = move(sections);
  copy_inherited();
}

void config::set_included(file_list included) {
  m_included = move(included);
}

file_list config::get_included_files() const {
  return m_included;
}

/**
 * Print a deprecation warning if the given parameter is set
 */
void config::warn_deprecated(const string& section, const string& key, string replacement) const {
  if (has(section, key)) {
    if (replacement.empty()) {
      m_log.warn(
          "The config parameter '%s.%s' is deprecated, it will be removed in the future. Please remove it from your "
          "config",
          section, key);
    } else {
      m_log.warn(
          "The config parameter `%s.%s` is deprecated, use `%s.%s` instead.", section, key, section, move(replacement));
    }
  }
}

/**
 * Returns true if a given parameter exists
 */
bool config::has(const string& section, const string& key) const {
  auto it = m_sections.find(section);
  return it != m_sections.end() && it->second.find(key) != it->second.end();
}

/**
 * Set parameter value
 */
void config::set(const string& section, const string& key, string&& value) {
  auto it = m_sections.find(section);
  if (it == m_sections.end()) {
    valuemap_t values;
    values[key] = value;
    m_sections[section] = move(values);
  }
  auto it2 = it->second.find(key);
  if ((it2 = it->second.find(key)) == it->second.end()) {
    it2 = it->second.emplace_hint(it2, key, value);
  } else {
    it2->second = value;
  }
}

/**
 * Look for sections set up to inherit from a base section
 * and copy the missing parameters
 *
 * Multiple sections can be specified, separated by a space.
 *
 *   [sub/section]
 *   inherit = section1 section2
 */
void config::copy_inherited() {
  for (auto&& section : m_sections) {
    std::vector<string> inherit_sections;

    // Collect all sections to be inherited
    for (auto&& param : section.second) {
      string key_name = param.first;
      if (key_name == "inherit") {
        auto inherit = param.second;
        inherit = dereference(section.first, key_name, inherit);

        std::vector<string> sections = string_util::split(std::move(inherit), ' ');

        inherit_sections.insert(inherit_sections.end(), sections.begin(), sections.end());

      } else if (key_name.find("inherit") == 0) {
        // Legacy support for keys that just start with 'inherit'
        m_log.warn(
            "\"%s.%s\": Using anything other than 'inherit' for inheriting section keys is deprecated. "
            "The 'inherit' key supports multiple section names separated by a space.",
            section.first, key_name);

        auto inherit = param.second;
        inherit = dereference(section.first, key_name, inherit);
        if (inherit.empty() || m_sections.find(inherit) == m_sections.end()) {
          throw value_error(
              "Invalid section \"" + inherit + "\" defined for \"" + section.first + "." + key_name + "\"");
        }

        inherit_sections.push_back(std::move(inherit));
      }
    }

    for (const auto& base_name : inherit_sections) {
      const auto base_section = m_sections.find(base_name);
      if (base_section == m_sections.end()) {
        throw value_error("Invalid section \"" + base_name + "\" defined for \"" + section.first + ".inherit\"");
      }

      m_log.trace("config: Inheriting keys from \"%s\" in \"%s\"", base_name, section.first);

      /*
       * Iterate the base and copy the parameters that haven't been defined
       * yet.
       */
      for (auto&& base_param : base_section->second) {
        section.second.emplace(base_param.first, base_param.second);
      }
    }
  }
}

/**
 * Dereference value reference
 */
string config::dereference(const string& section, const string& key, const string& var) const {
  if (var.substr(0, 2) != "${" || var.substr(var.length() - 1) != "}") {
    return var;
  }

  auto path = var.substr(2, var.length() - 3);
  size_t pos;

  if (path.compare(0, 4, "env:") == 0) {
    return dereference_env(path.substr(4));
  } else if (path.compare(0, 5, "xrdb:") == 0) {
    return dereference_xrdb(path.substr(5));
  } else if (path.compare(0, 5, "file:") == 0) {
    return dereference_file(path.substr(5));
  } else if ((pos = path.find(".")) != string::npos) {
    return dereference_local(path.substr(0, pos), path.substr(pos + 1), section);
  } else {
    throw value_error("Invalid reference defined at \"" + section + "." + key + "\"");
  }
}

/**
 * Dereference local value reference defined using:
 *  ${root.key}
 *  ${root.key:fallback}
 *  ${self.key}
 *  ${self.key:fallback}
 *  ${section.key}
 *  ${section.key:fallback}
 */
string config::dereference_local(string section, const string& key, const string& current_section) const {
  if (section == "BAR") {
    m_log.warn("${BAR.key} is deprecated. Use ${root.key} instead");
  }

  section = string_util::replace(section, "BAR", this->section(), 0, 3);
  section = string_util::replace(section, "root", this->section(), 0, 4);
  section = string_util::replace(section, "self", current_section, 0, 4);

  try {
    string string_value{get<string>(section, key)};
    return dereference(string(section), move(key), move(string_value));
  } catch (const key_error& err) {
    size_t pos;
    if ((pos = key.find(':')) != string::npos) {
      string fallback = key.substr(pos + 1);
      m_log.info("The reference ${%s.%s} does not exist, using defined fallback value \"%s\"", section,
          key.substr(0, pos), fallback);
      return fallback;
    }
    throw value_error("The reference ${" + section + "." + key + "} does not exist (no fallback set)");
  }
}

/**
 * Dereference environment variable reference defined using:
 *  ${env:key}
 *  ${env:key:fallback value}
 */
string config::dereference_env(string var) const {
  size_t pos;
  string env_default;
  /*
    * This is needed because with only the string we cannot distinguish
    * between an empty string as default and not default
    */
  bool has_default = false;

  if ((pos = var.find(':')) != string::npos) {
    env_default = var.substr(pos + 1);
    has_default = true;
    var.erase(pos);
  }

  if (env_util::has(var)) {
    string env_value{env_util::get(var)};
    m_log.info("Environment var reference ${%s} found (value=%s)", var, env_value);
    return env_value;
  } else if (has_default) {
    m_log.info("Environment var ${%s} is undefined, using defined fallback value \"%s\"", var, env_default);
    return env_default;
  } else {
    throw value_error(sstream() << "Environment var ${" << var << "} does not exist (no fallback set)");
  }
}

/**
 * Dereference X resource db value defined using:
 *  ${xrdb:key}
 *  ${xrdb:key:fallback value}
 */
string config::dereference_xrdb(string var) const {
  size_t pos;
#if not WITH_XRM
  m_log.warn("No built-in support to dereference ${xrdb:%s} references (requires `xcb-util-xrm`)", var);
  if ((pos = var.find(':')) != string::npos) {
    return var.substr(pos + 1);
  }
  return "";
#else
  if (!m_xrm) {
    throw application_error("xrm is not initialized");
  }

  string fallback;
  bool has_fallback = false;
  if ((pos = var.find(':')) != string::npos) {
    fallback = var.substr(pos + 1);
    has_fallback = true;
    var.erase(pos);
  }

  try {
    auto value = m_xrm->require<string>(var.c_str());
    m_log.info("Found matching X resource \"%s\" (value=%s)", var, value);
    return value;
  } catch (const xresource_error& err) {
    if (has_fallback) {
      m_log.info("%s, using defined fallback value \"%s\"", err.what(), fallback);
      return fallback;
    }
    throw value_error(sstream() << err.what() << " (no fallback set)");
  }
#endif
}

/**
 * Dereference file reference by reading its contents
 *  ${file:/absolute/file/path}
 *  ${file:/absolute/file/path:fallback value}
 */
string config::dereference_file(string var) const {
  size_t pos;
  string fallback;
  bool has_fallback = false;
  if ((pos = var.find(':')) != string::npos) {
    fallback = var.substr(pos + 1);
    has_fallback = true;
    var.erase(pos);
  }
  var = file_util::expand(var);

  if (file_util::exists(var)) {
    m_log.info("File reference \"%s\" found", var);
    return string_util::trim(file_util::contents(var), '\n');
  } else if (has_fallback) {
    m_log.info("File reference \"%s\" not found, using defined fallback value \"%s\"", var, fallback);
    return fallback;
  } else {
    throw value_error(sstream() << "The file \"" << var << "\" does not exist (no fallback set)");
  }
}


template <>
string config::convert(string&& value) const {
  return forward<string>(value);
}

template <>
const char* config::convert(string&& value) const {
  return value.c_str();
}

template <>
char config::convert(string&& value) const {
  return value.c_str()[0];
}

template <>
int config::convert(string&& value) const {
  return std::strtol(value.c_str(), nullptr, 10);
}

template <>
short config::convert(string&& value) const {
  return static_cast<short>(std::strtol(value.c_str(), nullptr, 10));
}

template <>
bool config::convert(string&& value) const {
  string lower{string_util::lower(forward<string>(value))};

  return (lower == "true" || lower == "yes" || lower == "on" || lower == "1");
}

template <>
float config::convert(string&& value) const {
  return std::strtof(value.c_str(), nullptr);
}

template <>
double config::convert(string&& value) const {
  return std::strtod(value.c_str(), nullptr);
}

template <>
long config::convert(string&& value) const {
  return std::strtol(value.c_str(), nullptr, 10);
}

template <>
long long config::convert(string&& value) const {
  return std::strtoll(value.c_str(), nullptr, 10);
}

template <>
unsigned char config::convert(string&& value) const {
  return std::strtoul(value.c_str(), nullptr, 10);
}

template <>
unsigned short config::convert(string&& value) const {
  return std::strtoul(value.c_str(), nullptr, 10);
}

template <>
unsigned int config::convert(string&& value) const {
  return std::strtoul(value.c_str(), nullptr, 10);
}

template <>
unsigned long config::convert(string&& value) const {
  unsigned long v{std::strtoul(value.c_str(), nullptr, 10)};
  return v < ULONG_MAX ? v : 0UL;
}

template <>
unsigned long long config::convert(string&& value) const {
  unsigned long long v{std::strtoull(value.c_str(), nullptr, 10)};
  return v < ULLONG_MAX ? v : 0ULL;
}

template <>
spacing_val config::convert(string&& value) const {
  return units_utils::parse_spacing(value);
}

template <>
extent_val config::convert(std::string&& value) const {
  return units_utils::parse_extent(value);
}

/**
 * Allows a new format for pixel sizes (like width in the bar section)
 *
 * The new format is X%:Z, where X is in [0, 100], and Z is any real value
 * describing a pixel offset. The actual value is calculated by X% * max + Z
 */
template <>
percentage_with_offset config::convert(string&& value) const {
  size_t i = value.find(':');

  if (i == std::string::npos) {
    if (value.find('%') != std::string::npos) {
      return {std::stod(value), {}};
    } else {
      return {0., convert<extent_val>(move(value))};
    }
  } else {
    std::string percentage = value.substr(0, i - 1);
    return {std::stod(percentage), convert<extent_val>(value.substr(i + 1))};
  }
}

template <>
chrono::seconds config::convert(string&& value) const {
  return chrono::seconds{convert<chrono::seconds::rep>(forward<string>(value))};
}

template <>
chrono::milliseconds config::convert(string&& value) const {
  return chrono::milliseconds{convert<chrono::milliseconds::rep>(forward<string>(value))};
}

template <>
chrono::duration<double> config::convert(string&& value) const {
  return chrono::duration<double>{convert<double>(forward<string>(value))};
}

template <>
rgba config::convert(string&& value) const {
  if (value.empty()) {
    return rgba{};
  }

  rgba ret{value};

  if (!ret.has_color()) {
    throw value_error("\"" + value + "\" is an invalid color value.");
  }

  return ret;
}

template <>
cairo_operator_t config::convert(string&& value) const {
  return cairo::utils::str2operator(forward<string>(value), CAIRO_OPERATOR_OVER);
}

POLYBAR_NS_END
