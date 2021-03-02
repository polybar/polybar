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
#include "utils/unit.hpp"

POLYBAR_NS

namespace chrono = std::chrono;

/**
 * Create instance
 */
config::make_type config::make(string path, string bar) {
  return *factory_util::singleton<std::remove_reference_t<config::make_type>>(logger::make(), move(path), move(bar));
}

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
  return "bar/" + m_barname;
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

/**
 * Print a deprecation warning if the given parameter is set
 */
void config::warn_deprecated(const string& section, const string& key, string replacement) const {
  try {
    auto value = get<string>(section, key);
    m_log.warn(
        "The config parameter `%s.%s` is deprecated, use `%s.%s` instead.", section, key, section, move(replacement));
  } catch (const key_error& err) {
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
        inherit = dereference<string>(section.first, key_name, inherit, inherit);

        std::vector<string> sections = string_util::split(std::move(inherit), ' ');

        inherit_sections.insert(inherit_sections.end(), sections.begin(), sections.end());

      } else if (key_name.find("inherit") == 0) {
        // Legacy support for keys that just start with 'inherit'
        m_log.warn(
            "\"%s.%s\": Using anything other than 'inherit' for inheriting section keys is deprecated. "
            "The 'inherit' key supports multiple section names separated by a space.",
            section.first, key_name);

        auto inherit = param.second;
        inherit = dereference<string>(section.first, key_name, inherit, inherit);
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
space_size config::convert(string&& value) const {
  char* new_end;
  auto size_value = std::strtof(value.c_str(), &new_end);

  if (size_value < 0) {
    throw application_error(sstream() << "Value: " << value << " must be positive ");
  }

  space_size size{space_type::SPACE, size_value};

  string unit = string_util::trim(new_end);
  if (!unit.empty()) {
    if (unit == "px") {
      size.type = space_type::PIXEL;
      size.value = std::trunc(size.value);
    } else if (unit == "pt") {
      size.type = space_type::POINT;
    } else {
      size.value = std::trunc(size.value);
    }
  }

  return size;
}

template <>
geometry config::convert(std::string&& value) const {
  return unit_utils::geometry_from_string(move(value));
}

/**
 * Allows a new format for pixel sizes (like width in the bar section)
 *
 * The new format is X%:Z, where X is in [0, 100], and Z is any real value
 * describing a pixel offset. The actual value is calculated by X% * max + Z
 */
template <>
geometry_format_values config::convert(string&& value) const {
  size_t i = value.find(':');

  if (i == std::string::npos) {
    if (value.find('%') != std::string::npos) {
      return {strtod(value.c_str(), nullptr), {}};
    } else {
      return {0., convert<geometry>(move(value))};
    }
  } else {
    std::string percentage = value.substr(0, i - 1);
    return {strtod(percentage.c_str(), nullptr), convert<geometry>(value.substr(i + 1))};
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
