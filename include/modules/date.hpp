#pragma once

#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  class date_module : public timer_module<date_module> {
   public:
    using timer_module::timer_module;

    void setup() {
      m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 1));

      m_formatter->add(DEFAULT_FORMAT, TAG_DATE, {TAG_DATE});

      m_format = m_conf.get<string>(name(), "date");
      m_formatalt = m_conf.get<string>(name(), "date-alt", "");
    }

    bool update() {
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

    string get_output() {
      if (!m_formatalt.empty())
        m_builder->cmd(mousebtn::LEFT, EVENT_TOGGLE);
      m_builder->node(timer_module::get_output());
      return m_builder->flush();
    }

    bool build(builder* builder, string tag) {
      if (tag == TAG_DATE)
        builder->node(m_buffer);
      return tag == TAG_DATE;
    }

    bool handle_event(string cmd) {
      if (cmd == EVENT_TOGGLE) {
        m_toggled = !m_toggled;
        wakeup();
      }
      return cmd == EVENT_TOGGLE;
    }

    bool receive_events() const {
      return true;
    }

   private:
    static constexpr auto TAG_DATE = "<date>";

    static constexpr auto EVENT_TOGGLE = "datetoggle";

    string m_format;
    string m_formatalt;

    char m_buffer[256] = {'\0'};
    stateflag m_toggled{false};
  };
}

LEMONBUDDY_NS_END
