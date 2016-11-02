#include "components/config.hpp"
#include "utils/file.hpp"

LEMONBUDDY_NS

/**
 * Load configuration and validate bar section
 *
 * This is done outside the constructor due to boost::di noexcept
 */
void config::load(string file, string barname) {
  m_file = file;
  m_current_bar = barname;

  if (!file_util::exists(file))
    throw application_error("Could not find config file: " + file);

  try {
    boost::property_tree::read_ini(file, m_ptree);
  } catch (const std::exception& e) {
    throw application_error(e.what());
  }

  auto bars = defined_bars();
  if (std::find(bars.begin(), bars.end(), m_current_bar) == bars.end())
    throw application_error("Undefined bar: " + m_current_bar);

  if (has_env("XDG_CONFIG_HOME"))
    file = string_util::replace(file, read_env("XDG_CONFIG_HOME"), "$XDG_CONFIG_HOME");
  if (has_env("HOME"))
    file = string_util::replace(file, read_env("HOME"), "~");
  m_logger.trace("config: Loaded %s", file);
  m_logger.trace("config: Current bar section: [%s]", bar_section());
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
    if (p.first.compare(0, 4, "bar/") == 0)
      bars.emplace_back(p.first.substr(4));
  }

  return bars;
}

/**
 * Build path used to find a parameter in the given section
 */
string config::build_path(const string& section, const string& key) const {
  return section + "." + key;
}

LEMONBUDDY_NS_END
