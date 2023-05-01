#include "modules/temperature.hpp"

#include "drawtypes/label.hpp"
#include "drawtypes/ramp.hpp"
#include "utils/file.hpp"
#include "utils/math.hpp"
#include <cmath>

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<temperature_module>;

  temperature_module::temperature_module(const bar_settings& bar, string name_, const config& config)
      : timer_module<temperature_module>(bar, move(name_), config) {
    m_zone = m_conf.get(name(), "thermal-zone", 0);
    m_zone_type = m_conf.get(name(), "zone-type", ""s);
    m_path = m_conf.get(name(), "hwmon-path", ""s);
    m_tempbase = m_conf.get(name(), "base-temperature", 0);
    m_tempwarn = m_conf.get(name(), "warn-temperature", 80);
    set_interval(1s);
    m_units = m_conf.get(name(), "units", m_units);

    if (!m_zone_type.empty()) {
      bool zone_found = false;
      vector<string> zone_paths = file_util::glob(PATH_THERMAL_ZONE_WILDCARD);
      vector<string> available_zones;
      for (auto &z: zone_paths) {
        string zone_file = z + "/type";
        string z_zone_type = string_util::strip_trailing_newline(file_util::contents(zone_file));
        available_zones.push_back(z_zone_type);
        if (z_zone_type == m_zone_type) {
          m_path = z + "/temp";
          zone_found = true;
          break;
        }
      }

      if (!zone_found) {
        throw module_error("zone-type '" + m_zone_type +  "' was not found, available zone types: " + string_util::join(available_zones, ", "));
      }
    }

    if (m_path.empty()) {
      m_path = string_util::replace(PATH_TEMPERATURE_INFO, "%zone%", to_string(m_zone));
    }

    if (!file_util::exists(m_path)) {
      throw module_error("The file '" + m_path + "' does not exist");
    }

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_RAMP});
    m_formatter->add(FORMAT_WARN, TAG_LABEL_WARN, {TAG_LABEL_WARN, TAG_RAMP});

    if (m_formatter->has(TAG_LABEL)) {
      m_label[temp_state::NORMAL] = load_optional_label(m_conf, name(), TAG_LABEL, "%temperature-c%");
    }
    if (m_formatter->has(TAG_LABEL_WARN)) {
      m_label[temp_state::WARN] = load_optional_label(m_conf, name(), TAG_LABEL_WARN, "%temperature-c%");
    }
    if (m_formatter->has(TAG_RAMP)) {
      m_ramp = load_ramp(m_conf, name(), TAG_RAMP);
    }

    // Deprecation warning for the %temperature% token
    if((m_label[temp_state::NORMAL] && m_label[temp_state::NORMAL]->has_token("%temperature%")) ||
        ((m_label[temp_state::WARN] && m_label[temp_state::WARN]->has_token("%temperature%")))) {
      m_log.warn("%s: The token `%%temperature%%` is deprecated, use `%%temperature-c%%` instead.", name());
    }
  }

  bool temperature_module::update() {
    float temp = float(std::strtol(file_util::contents(m_path).c_str(), nullptr, 10)) / 1000.0;
    m_temp     = std::lround( temp );
    int temp_f = std::lround( (temp * 1.8) + 32.0 );
    int temp_k = std::lround( temp + 273.15 );

    string temp_c_string = to_string(m_temp);
    string temp_f_string = to_string(temp_f);
    string temp_k_string = to_string(temp_k);

    // Add units if `units = true` in config
    if(m_units) {
      temp_c_string += "°C";
      temp_f_string += "°F";
      temp_k_string += "K";
    }

    const auto replace_tokens = [&](label_t& label) {
      label->reset_tokens();
      label->replace_token("%temperature-f%", temp_f_string);
      label->replace_token("%temperature-c%", temp_c_string);
      label->replace_token("%temperature-k%", temp_k_string);

      // DEPRECATED: Will be removed in later release
      label->replace_token("%temperature%", temp_c_string);
    };

    if (m_label[temp_state::NORMAL]) {
      replace_tokens(m_label[temp_state::NORMAL]);
    }
    if (m_label[temp_state::WARN]) {
      replace_tokens(m_label[temp_state::WARN]);
    }

    return true;
  }

  string temperature_module::get_format() const {
    if (m_temp >= m_tempwarn) {
      return FORMAT_WARN;
    } else {
      return DEFAULT_FORMAT;
    }
  }

  bool temperature_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL) {
      builder->node(m_label.at(temp_state::NORMAL));
    } else if (tag == TAG_LABEL_WARN) {
      builder->node(m_label.at(temp_state::WARN));
    } else if (tag == TAG_RAMP) {
      builder->node(m_ramp->get_by_percentage_with_borders(m_temp, m_tempbase, m_tempwarn));
    } else {
      return false;
    }
    return true;
  }
}

POLYBAR_NS_END
