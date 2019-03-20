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

  temperature_module::temperature_module(const bar_settings& bar, string name_)
      : timer_module<temperature_module>(bar, move(name_)) {
    m_zone = m_conf.get(name(), "thermal-zone", 0);
    m_path = m_conf.get(name(), "hwmon-path", ""s);
    m_tempbase = m_conf.get(name(), "base-temperature", 0);
    m_tempwarn = m_conf.get(name(), "warn-temperature", 80);
    m_interval = m_conf.get<decltype(m_interval)>(name(), "interval", 1s);
    m_units = m_conf.get(name(), "units", m_units);

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
    m_temp = std::strtol(file_util::contents(m_path).c_str(), nullptr, 10) / 1000.0f + 0.5f;
    int temp_f = floor(((1.8 * m_temp) + 32) + 0.5);
    m_perc = math_util::cap(math_util::percentage(m_temp, m_tempbase, m_tempwarn), 0, 100);

    string temp_c_string = to_string(m_temp);
    string temp_f_string = to_string(temp_f);

    // Add units if `units = true` in config
    if(m_units) {
      temp_c_string += "°C";
      temp_f_string += "°F";
    }

    const auto replace_tokens = [&](label_t& label) {
      label->reset_tokens();
      label->replace_token("%temperature-f%", temp_f_string);
      label->replace_token("%temperature-c%", temp_c_string);

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
    if (m_temp > m_tempwarn) {
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
      builder->node(m_ramp->get_by_percentage(m_perc));
    } else {
      return false;
    }
    return true;
  }
}

POLYBAR_NS_END
