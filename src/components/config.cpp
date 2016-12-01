#include <algorithm>
#include <utility>

#include "components/config.hpp"
#include "utils/env.hpp"
#include "utils/file.hpp"

POLYBAR_NS

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

  try {
    boost::property_tree::read_ini(file, m_ptree);
  } catch (const std::exception& e) {
    throw application_error(e.what());
  }

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
 * Look for sections set up to inherit from a base section
 * and copy the missing parameters
 *
 *   [sub/seciton]
 *   inherit = base/section
 */
void config::copy_inherited() {
  for (auto&& section : m_ptree) {
    for (auto&& param : section.second) {
      if (param.first.compare(KEY_INHERIT) == 0) {
        // Get name of base section
        auto inherit = param.second.get_value<string>();
        if ((inherit = dereference<string>(section.first, param.first, inherit, inherit)).empty()) {
          throw value_error("[" + section.first + "." + KEY_INHERIT + "] requires a value");
        }

        // Find and validate base section
        auto base_section = m_ptree.get_child_optional(inherit);
        if (!base_section || base_section.value().empty()) {
          throw value_error("[" + section.first + "." + KEY_INHERIT + "] invalid reference \"" + inherit + "\"");
        }

        m_logger.trace("config: Copying missing params (sub=\"%s\", base=\"%s\")", section.first, inherit);

        // Iterate the the base and copy the parameters
        // that hasn't been defined for the sub-section
        for (auto&& base_param : *base_section) {
          if (!section.second.get_child_optional(base_param.first)) {
            section.second.put_child(base_param.first, base_param.second);
          }
        }
      }
    }
  }
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

  for (auto&& p : m_ptree) {
    if (p.first.compare(0, 4, "bar/") == 0) {
      bars.emplace_back(p.first.substr(4));
    }
  }

  return bars;
}

/**
 * Build path used to find a parameter in the given section
 */
string config::build_path(const string& section, const string& key) const {
  return section + "." + key;
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

POLYBAR_NS_END
