#pragma once

#include "modules/meta/timer_module.hpp"

POLYBAR_NS

namespace modules {
  class date_module : public timer_module<date_module> {
   public:
    using timer_module::timer_module;

    void setup();
    bool update();
    bool build(builder* builder, const string& tag) const;
    bool handle_event(string cmd);
    bool receive_events() const {
      return true;
    }

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto EVENT_TOGGLE = "datetoggle";

    // @deprecated: Use <label>
    static constexpr auto TAG_DATE = "<date>";

    label_t m_label;

    string m_dateformat;
    string m_dateformat_alt;
    string m_timeformat;
    string m_timeformat_alt;

    string m_date;
    string m_time;

    stateflag m_toggled{false};
  };
}

POLYBAR_NS_END
