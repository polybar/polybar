#pragma once

#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  class date_module : public timer_module<date_module> {
   public:
    using timer_module::timer_module;

    void setup();
    bool update();
    bool build(builder* builder, string tag) const;
    bool handle_event(string cmd);
    bool receive_events() const;

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
