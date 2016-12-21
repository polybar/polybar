#pragma once

#include "modules/meta/input_handler.hpp"
#include "modules/meta/timer_module.hpp"

POLYBAR_NS

namespace modules {
  class date_module : public timer_module<date_module>, public input_handler {
   public:
    explicit date_module(const bar_settings&, string);

    bool update();
    bool build(builder* builder, const string& tag) const;

   protected:
    bool on(const input_event_t& evt);

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
