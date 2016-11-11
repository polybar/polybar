#include "modules/temperature.hpp"
#include "utils/file.hpp"
#include "utils/math.hpp"

LEMONBUDDY_NS

namespace modules {
  void temperature_module::setup() {
    m_zone = m_conf.get<int>(name(), "thermal-zone", 0);
    m_tempwarn = m_conf.get<int>(name(), "warn-temperature", 80);
    m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 1));

    m_path = string_util::replace(PATH_TEMPERATURE_INFO, "%zone%", to_string(m_zone));

    if (!file_util::exists(m_path))
      throw module_error("The file '" + m_path + "' does not exist");

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_RAMP});
    m_formatter->add(FORMAT_WARN, TAG_LABEL_WARN, {TAG_LABEL_WARN, TAG_RAMP});

    if (m_formatter->has(TAG_LABEL))
      m_label[temp_state::NORMAL] = load_optional_label(m_conf, name(), TAG_LABEL, "%temperature%");
    if (m_formatter->has(TAG_LABEL_WARN))
      m_label[temp_state::WARN] = load_optional_label(m_conf, name(), TAG_LABEL_WARN, "%temperature%");
    if (m_formatter->has(TAG_RAMP))
      m_ramp = load_ramp(m_conf, name(), TAG_RAMP);
  }

  bool temperature_module::update() {
    m_temp = std::atoi(file_util::get_contents(m_path).c_str()) / 1000.0f + 0.5f;

    const auto replace_tokens = [&](label_t& label) {
      label->reset_tokens();
      label->replace_token("%temperature%", to_string(m_temp) + "°C");
    };

    if (m_label[temp_state::NORMAL])
      replace_tokens(m_label[temp_state::NORMAL]);
    if (m_label[temp_state::WARN])
      replace_tokens(m_label[temp_state::WARN]);

    return true;
  }

  string temperature_module::get_format() const {
    if (m_temp > m_tempwarn)
      return FORMAT_WARN;
    else
      return DEFAULT_FORMAT;
  }

  bool temperature_module::build(builder* builder, string tag) const {
    if (tag == TAG_LABEL)
      builder->node(m_label.at(temp_state::NORMAL));
    else if (tag == TAG_LABEL_WARN)
      builder->node(m_label.at(temp_state::WARN));
    else if (tag == TAG_RAMP)
      builder->node(m_ramp->get_by_percentage(math_util::cap(m_temp, 0, m_tempwarn)));
    else
      return false;
    return true;
  }
}

LEMONBUDDY_NS_END
