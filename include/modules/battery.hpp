#pragma once

#include "common.hpp"
#include "modules/meta/inotify_module.hpp"
#include "modules/meta/types.hpp"

POLYBAR_NS

namespace modules {
  class battery_module : public inotify_module<battery_module> {
   public:
    enum class state {
      NONE = 0,
      CHARGING,
      DISCHARGING,
      LOW,
      FULL,
    };

    enum class value {
      NONE = 0,
      ADAPTER,
      CAPACITY,
      CAPACITY_MAX,
      VOLTAGE,
      RATE,
    };

    template <typename ReturnType>
    struct value_reader {
      using return_type = ReturnType;

      explicit value_reader() = default;
      explicit value_reader(function<ReturnType()>&& fn) : m_fn(forward<decltype(fn)>(fn)) {}

      ReturnType read() const {
        return m_fn();
      }

     private:
      const function<ReturnType()> m_fn;
    };

    using state_reader = mutex_wrapper<value_reader<bool /* is_charging */>>;
    using capacity_reader = mutex_wrapper<value_reader<int /* percentage */>>;
    using rate_reader = mutex_wrapper<value_reader<unsigned long /* seconds */>>;
    using consumption_reader = mutex_wrapper<value_reader<string /* watts */>>;

   public:
    explicit battery_module(const bar_settings&, string, const config&);

    void start() override;
    void teardown();
    void idle();
    bool on_event(const inotify_event& event);
    string get_format() const;
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = BATTERY_TYPE;

   protected:
    state current_state();
    int current_percentage();
    int clamp_percentage(int percentage, state state) const;
    string current_time();
    string current_consumption();
    void subthread();

   private:
    static constexpr const char* FORMAT_CHARGING{"format-charging"};
    static constexpr const char* FORMAT_DISCHARGING{"format-discharging"};
    static constexpr const char* FORMAT_FULL{"format-full"};
    static constexpr const char* FORMAT_LOW{"format-low"};

    static constexpr const char* TAG_ANIMATION_CHARGING{"<animation-charging>"};
    static constexpr const char* TAG_ANIMATION_DISCHARGING{"<animation-discharging>"};
    static constexpr const char* TAG_ANIMATION_LOW{"<animation-low>"};
    static constexpr const char* TAG_BAR_CAPACITY{"<bar-capacity>"};
    static constexpr const char* TAG_RAMP_CAPACITY{"<ramp-capacity>"};
    static constexpr const char* TAG_LABEL_CHARGING{"<label-charging>"};
    static constexpr const char* TAG_LABEL_DISCHARGING{"<label-discharging>"};
    static constexpr const char* TAG_LABEL_FULL{"<label-full>"};
    static constexpr const char* TAG_LABEL_LOW{"<label-low>"};

    static const size_t SKIP_N_UNCHANGED{3_z};

    unique_ptr<state_reader> m_state_reader;
    unique_ptr<capacity_reader> m_capacity_reader;
    unique_ptr<rate_reader> m_rate_reader;
    unique_ptr<consumption_reader> m_consumption_reader;

    label_t m_label_charging;
    label_t m_label_discharging;
    label_t m_label_full;
    label_t m_label_low;
    animation_t m_animation_charging;
    animation_t m_animation_discharging;
    animation_t m_animation_low;
    progressbar_t m_bar_capacity;
    ramp_t m_ramp_capacity;

    string m_fstate;
    string m_fcapnow;
    string m_fcapfull;
    string m_frate;
    string m_fvoltage;

    state m_state{state::DISCHARGING};
    int m_percentage{0};

    int m_fullat{100};
    int m_lowat{10};
    string m_timeformat;
    size_t m_unchanged{SKIP_N_UNCHANGED};
    chrono::duration<double> m_interval{};
    chrono::steady_clock::time_point m_lastpoll;
    thread m_subthread;
  };
} // namespace modules

POLYBAR_NS_END
