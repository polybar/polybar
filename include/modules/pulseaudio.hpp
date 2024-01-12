#pragma once

#include "modules/meta/event_module.hpp"
#include "modules/meta/types.hpp"
#include "settings.hpp"

POLYBAR_NS

// fwd
class pulseaudio;

namespace modules {
  using pulseaudio_t = shared_ptr<pulseaudio>;

  class pulseaudio_module : public event_module<pulseaudio_module> {
   public:
    explicit pulseaudio_module(const bar_settings&, string, const config&);

    void teardown();
    bool has_event();
    bool update();
    string get_format() const;
    string get_output();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = PULSEAUDIO_TYPE;

    static constexpr auto EVENT_INC = "inc";
    static constexpr auto EVENT_DEC = "dec";
    static constexpr auto EVENT_TOGGLE = "toggle";

   protected:
    void action_inc();
    void action_dec();
    void action_toggle();

   private:
    static constexpr auto FORMAT_VOLUME = "format-volume";
    static constexpr auto FORMAT_MUTED = "format-muted";

    static constexpr auto TAG_RAMP_VOLUME = "<ramp-volume>";
    static constexpr auto TAG_BAR_VOLUME = "<bar-volume>";
    static constexpr auto TAG_LABEL_VOLUME = "<label-volume>";
    static constexpr auto TAG_LABEL_MUTED = "<label-muted>";

    progressbar_t m_bar_volume;
    ramp_t m_ramp_volume;
    label_t m_label_volume;
    label_t m_label_muted;

    pulseaudio_t m_pulseaudio;

    int m_interval{5};
    bool m_unmute_on_scroll{false};
    atomic<bool> m_muted{false};
    atomic<int> m_volume{0};
    atomic<double> m_decibels{0};
    atomic<bool> m_reverse_scroll{false};
  };
}  // namespace modules

POLYBAR_NS_END
