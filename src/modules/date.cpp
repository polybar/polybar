#include "modules/date.hpp"

LEMONBUDDY_NS

namespace modules {
  void date_module::setup() {
    if (!m_bar.locale.empty())
      setlocale(LC_TIME, m_bar.locale.c_str());

    m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 1));

    m_formatter->add(DEFAULT_FORMAT, TAG_DATE, {TAG_DATE});

    m_format = m_conf.get<string>(name(), "date");
    m_formatalt = m_conf.get<string>(name(), "date-alt", "");
  }

  bool date_module::update() {
    if (!m_formatter->has(TAG_DATE))
      return false;

    auto time = std::time(nullptr);
    auto date_format = m_toggled ? m_formatalt : m_format;
    char buffer[256] = {'\0'};

    std::strftime(buffer, sizeof(buffer), date_format.c_str(), std::localtime(&time));

    if (std::strncmp(buffer, m_buffer, sizeof(buffer)) == 0)
      return false;
    else
      std::memmove(m_buffer, buffer, sizeof(buffer));

    return true;
  }

  bool date_module::build(builder* builder, string tag) const {
    if (tag != TAG_DATE) {
      return false;
    }

    if (!m_formatalt.empty())
      m_builder->cmd(mousebtn::LEFT, EVENT_TOGGLE);

    builder->node(m_buffer);

    return true;
  }

  bool date_module::handle_event(string cmd) {
    if (cmd == EVENT_TOGGLE) {
      m_toggled = !m_toggled;
      wakeup();
    }

    return cmd == EVENT_TOGGLE;
  }

  bool date_module::receive_events() const {
    return true;
  }
}

LEMONBUDDY_NS_END
