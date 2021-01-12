#pragma once

#include <ctime>
#include <iomanip>
#include <iostream>

#include "modules/meta/timer_module.hpp"

POLYBAR_NS

namespace modules {
  class date_module : public timer_module<date_module> {
   public:
    explicit date_module(const bar_settings&, string);

    bool update();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = "internal/date";

    static constexpr auto EVENT_TOGGLE = "toggle";

   protected:
    bool input(const string& action, const string& data);

   private:
    static constexpr auto TAG_LABEL = "<label>";

    // \deprecated: Use <label>
    static constexpr auto TAG_DATE = "<date>";

    label_t m_label;

    string m_dateformat;
    string m_dateformat_alt;
    string m_timeformat;
    string m_timeformat_alt;

    string m_date;
    string m_time;

    // Single stringstream to be used to gather the results of std::put_time
    std::stringstream datetime_stream;

    std::atomic<bool> m_toggled{false};
  };
}  // namespace modules

POLYBAR_NS_END
