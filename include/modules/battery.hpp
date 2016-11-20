#pragma once

#include "common.hpp"
#include "config.hpp"
#include "modules/meta/inotify_module.hpp"

POLYBAR_NS

namespace modules {
  enum class battery_state {
    NONE = 0,
    CHARGING,
    DISCHARGING,
    FULL,
  };

  enum class battery_value {
    NONE = 0,
    ADAPTER,
    CAPACITY,
    CAPACITY_MAX,
    CAPACITY_PERC,
    VOLTAGE,
    RATE,
  };

  class battery_module : public inotify_module<battery_module> {
   public:
    using inotify_module::inotify_module;

    void setup();
    void start();
    void teardown();
    void idle();
    bool on_event(inotify_event* event);
    string get_format() const;
    bool build(builder* builder, string tag) const;

   protected:
    int current_percentage();
    battery_state current_state();
    string current_time();
    void subthread();

   private:
    static constexpr auto FORMAT_CHARGING = "format-charging";
    static constexpr auto FORMAT_DISCHARGING = "format-discharging";
    static constexpr auto FORMAT_FULL = "format-full";

    static constexpr auto TAG_ANIMATION_CHARGING = "<animation-charging>";
    static constexpr auto TAG_BAR_CAPACITY = "<bar-capacity>";
    static constexpr auto TAG_RAMP_CAPACITY = "<ramp-capacity>";
    static constexpr auto TAG_LABEL_CHARGING = "<label-charging>";
    static constexpr auto TAG_LABEL_DISCHARGING = "<label-discharging>";
    static constexpr auto TAG_LABEL_FULL = "<label-full>";

    static const int SKIP_N_UNCHANGED{3};

    animation_t m_animation_charging;
    ramp_t m_ramp_capacity;
    progressbar_t m_bar_capacity;
    label_t m_label_charging;
    label_t m_label_discharging;
    label_t m_label_full;

    battery_state m_state{battery_state::DISCHARGING};
    map<battery_value, string> m_valuepath;
    std::atomic<int> m_percentage{0};
    int m_fullat{100};
    chrono::duration<double> m_interval;
    chrono::system_clock::time_point m_lastpoll;
    string m_timeformat;
    int m_unchanged{SKIP_N_UNCHANGED};
  };
}

POLYBAR_NS_END
