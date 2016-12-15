#include <algorithm>
#include <chrono>
#include <fstream>
#include <istream>
#include <utility>

#include "components/config.hpp"
#include "utils/env.hpp"
#include "utils/factory.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace chrono = std::chrono;

/**
 * Create instance
 */
config::make_type config::make(string path, string bar) {
  return static_cast<config::make_type>(*factory_util::singleton<std::remove_reference_t<config::make_type>>(
      logger::make(), xresource_manager::make(), move(path), move(bar)));
}

/**
 * Construct config object
 */
config::config(const logger& logger, const xresource_manager& xrm, string&& path, string&& bar)
    : m_log(logger), m_xrm(xrm), m_file(forward<string>(path)), m_barname(forward<string>(bar)) {
  if (!file_util::exists(m_file)) {
    throw application_error("Could not find config file: " + m_file);
  }

  parse_file();

  bool found_bar{false};
  for (auto&& p : m_sections) {
    if (p.first == section()) {
      found_bar = true;
      break;
    }
  }

  if (!found_bar) {
    throw application_error("Undefined bar: " + m_barname);
  }

  m_log.trace("config: Loaded %s", m_file);
  m_log.trace("config: Current bar section: [%s]", section());
}

/**
 * Get path of loaded file
 */
string config::filepath() const {
  return m_file;
}

/**
 * Get the section name of the bar in use
 */
string config::section() const {
  return "bar/" + m_barname;
}

/**
 * Print a deprecation warning if the given parameter is set
 */
void config::warn_deprecated(const string& section, const string& key, string replacement) const {
  try {
    auto value = get<string>(section, key);
    m_log.warn("The config parameter `%s.%s` is deprecated, use `%s` instead.", section, key, move(replacement));
  } catch (const key_error& err) {
  }
}

/**
 * Parse key/value pairs from the configuration file
 */
void config::parse_file() {
  std::ifstream in(m_file);
  string line;
  string section;
  uint32_t lineno{0};

  while (std::getline(in, line)) {
    lineno++;

    line = string_util::replace_all(line, "\t", "");

    // Ignore empty lines and comments
    if (line.empty() || line[0] == ';' || line[0] == '#') {
      continue;
    }

    // New section
    if (line[0] == '[' && line[line.length() - 1] == ']') {
      section = line.substr(1, line.length() - 2);
      continue;
    } else if (section.empty()) {
      continue;
    }

    size_t equal_pos;

    // Check for key-value pair equal sign
    if ((equal_pos = line.find('=')) == string::npos) {
      continue;
    }

    string key{forward<string>(string_util::trim(forward<string>(line.substr(0, equal_pos)), ' '))};
    string value;

    auto it = m_sections[section].find(key);
    if (it != m_sections[section].end()) {
      throw key_error("Duplicate key name \"" + key + "\" defined on line " + to_string(lineno));
    }

    if (equal_pos + 1 < line.size()) {
      value = string_util::trim(forward<string>(string_util::trim(line.substr(equal_pos + 1), ' ')), '"');
    }

    m_sections[section].emplace_hint(it, move(key), move(value));
  }

  copy_inherited();
}

/**
 * Look for sections set up to inherit from a base section
 * and copy the missing parameters
 *
 *   [sub/seciton]
 *   inherit = base/section
 */
void config::copy_inherited() {
  for (auto&& section : m_sections) {
    for (auto&& param : section.second) {
      if (param.first.compare(0, strlen(KEY_INHERIT), KEY_INHERIT) == 0) {
        // Get name of base section
        auto inherit = param.second;
        if ((inherit = dereference<string>(section.first, param.first, inherit, inherit)).empty()) {
          throw value_error("[" + section.first + "." + KEY_INHERIT + "] requires a value");
        }

        // Find and validate base section
        auto base_section = m_sections.find(inherit);
        if (base_section == m_sections.end()) {
          throw value_error("[" + section.first + "." + KEY_INHERIT + "] invalid reference \"" + inherit + "\"");
        }

        m_log.trace("config: Copying missing params (sub=\"%s\", base=\"%s\")", section.first, inherit);

        // Iterate the base and copy the parameters
        // that hasn't been defined for the sub-section
        for (auto&& base_param : base_section->second) {
          valuemap_t::const_iterator iter;
          section.second.insert(make_pair(base_param.first, base_param.second));
        }
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
  return std::atoi(value.c_str());
}

template <>
short config::convert(string&& value) const {
  return static_cast<short>(std::atoi(value.c_str()));
}

template <>
bool config::convert(string&& value) const {
  string lower{string_util::lower(forward<string>(value))};

  if (lower == "true") {
    return true;
  } else if (lower == "yes") {
    return true;
  } else if (lower == "on") {
    return true;
  } else if (lower == "1") {
    return true;
  } else {
    return false;
  }
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
  int conv{convert<int>(forward<string>(value))};
  if (conv < std::numeric_limits<unsigned char>::min()) {
    return std::numeric_limits<unsigned char>::min();
  } else if (conv > std::numeric_limits<unsigned char>::max()) {
    return std::numeric_limits<unsigned char>::max();
  } else {
    return static_cast<unsigned char>(conv);
  }
}

template <>
unsigned short config::convert(string&& value) const {
  int conv{convert<int>(forward<string>(value))};
  if (conv < std::numeric_limits<unsigned short>::min()) {
    return std::numeric_limits<unsigned short>::min();
  } else if (conv > std::numeric_limits<unsigned short>::max()) {
    return std::numeric_limits<unsigned short>::max();
  } else {
    return static_cast<unsigned short>(conv);
  }
}

template <>
unsigned int config::convert(string&& value) const {
  long conv{convert<int>(forward<string>(value))};
  if (conv < std::numeric_limits<unsigned int>::min()) {
    return std::numeric_limits<unsigned int>::min();
  } else if (conv > std::numeric_limits<unsigned int>::max()) {
    return std::numeric_limits<unsigned int>::max();
  } else {
    return static_cast<unsigned int>(conv);
  }
}

template <>
unsigned long config::convert(string&& value) const {
  return std::strtoul(value.c_str(), nullptr, 10);
}

template <>
unsigned long long config::convert(string&& value) const {
  return std::strtoull(value.c_str(), nullptr, 10);
}

template <>
chrono::seconds config::convert(string&& value) const {
  return chrono::seconds{convert<chrono::seconds::rep>(forward<string>(value))};
}

template <>
chrono::milliseconds config::convert(string&& value) const {
  return chrono::milliseconds{convert<chrono::milliseconds::rep>(forward<string>(value))};
}

POLYBAR_NS_END
