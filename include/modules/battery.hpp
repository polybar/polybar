#pragma once

#include "config.hpp"
#include "drawtypes/animation.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta.hpp"
#include "utils/file.hpp"
#include "utils/inotify.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

namespace modules {
  enum class battery_state { NONE = 0, UNKNOWN, CHARGING, DISCHARGING, FULL };
  class battery_module : public inotify_module<battery_module> {
   public:
    using inotify_module::inotify_module;

    void setup();
    void start();
    void teardown();
    bool on_event(inotify_event* event);
    string get_format() const;
    bool build(builder* builder, string tag) const;

   protected:
    int current_percentage();
    battery_state current_state();
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

    animation_t m_animation_charging;
    ramp_t m_ramp_capacity;
    progressbar_t m_bar_capacity;
    label_t m_label_charging;
    label_t m_label_discharging;
    label_t m_label_full;

    string m_battery;
    string m_adapter;
    string m_path_capacity;
    string m_path_adapter;

    battery_state m_state = battery_state::UNKNOWN;
    std::atomic_int m_percentage{0};

    stateflag m_notified{false};

    int m_fullat = 100;
  };
}

LEMONBUDDY_NS_END
