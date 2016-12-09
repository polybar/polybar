#include <algorithm>
#include <fstream>
#include <istream>
#include <utility>

#include "components/config.hpp"
#include "utils/env.hpp"
#include "utils/factory.hpp"
#include "utils/file.hpp"

POLYBAR_NS

/**
 * Create instance
 */
const config& config::make() {
  shared_ptr<config> instance = factory_util::singleton<config>(logger::make(), xresource_manager::make());
  return static_cast<config&>(*instance);
}

/**
 * Load configuration and validate bar section
 *
 * This is done outside the constructor due to boost::di noexcept
 */
void config::load(string file, string barname) {
  m_file = file;
  m_current_bar = move(barname);

  if (!file_util::exists(file)) {
    throw application_error("Could not find config file: " + file);
  }

  // Read values
  read();

  auto bars = defined_bars();
  if (std::find(bars.begin(), bars.end(), m_current_bar) == bars.end()) {
    throw application_error("Undefined bar: " + m_current_bar);
  }

  if (env_util::has("XDG_CONFIG_HOME")) {
    file = string_util::replace(file, env_util::get("XDG_CONFIG_HOME"), "$XDG_CONFIG_HOME");
  }
  if (env_util::has("HOME")) {
    file = string_util::replace(file, env_util::get("HOME"), "~");
  }

  m_logger.trace("config: Loaded %s", file);
  m_logger.trace("config: Current bar section: [%s]", bar_section());

  copy_inherited();
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
string config::bar_section() const {
  return "bar/" + m_current_bar;
}

/**
 * Get a list of defined bar sections in the current config
 */
vector<string> config::defined_bars() const {
  vector<string> bars;

  for (auto&& p : m_sections) {
    if (p.first.compare(0, 4, "bar/") == 0) {
      bars.emplace_back(p.first.substr(4));
    }
  }

  return bars;
}

/**
 * Print a deprecation warning if the given parameter is set
 */
void config::warn_deprecated(const string& section, const string& key, string replacement) const {
  try {
    auto value = get<string>(section, key);
    m_logger.warn("The config parameter `%s.%s` is deprecated, use `%s` instead.", section, key, move(replacement));
  } catch (const key_error& err) {
  }
}

void config::read() {
  std::ifstream in(m_file.c_str());
  string line;
  string section;

  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }

    // New section
    if (line[0] == '[' && line[line.length() - 1] == ']') {
      section = line.substr(1, line.length() - 2);
      continue;
    }

    size_t equal_pos;

    // Check for key-value pair equal sign
    if ((equal_pos = line.find('=')) == string::npos) {
      continue;
    }

    string key{string_util::trim(line.substr(0, equal_pos), ' ')};
    string value{string_util::trim(string_util::trim(line.substr(equal_pos + 1), ' '), '"')};

    m_sections[section][key] = move(value);
  }
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
      if (param.first.compare(KEY_INHERIT) == 0) {
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

        m_logger.trace("config: Copying missing params (sub=\"%s\", base=\"%s\")", section.first, inherit);

        // Iterate the base and copy the parameters
        // that hasn't been defined for the sub-section
        for (auto&& base_param : base_section->second) {
          valuemap_t::const_iterator iter;

          if ((iter = section.second.find(base_param.first)) == section.second.end()) {
            section.second.emplace_hint(iter, base_param.first, base_param.second);
          }
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
  return std::atoi(value.c_str()) != 0;
}

template <>
float config::convert(string&& value) const {
  return std::atof(value.c_str());
}

template <>
double config::convert(string&& value) const {
  return std::atof(value.c_str());
}

template <>
long config::convert(string&& value) const {
  return std::atol(value.c_str());
}

template <>
long long config::convert(string&& value) const {
  return std::atoll(value.c_str());
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

POLYBAR_NS_END
