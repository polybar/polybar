#pragma once

#include "settings.hpp"
#include "modules/meta/event_module.hpp"
#include "modules/meta/input_handler.hpp"

POLYBAR_NS

// fwd
class pulseaudio;

namespace modules {
  using pulseaudio_t = shared_ptr<pulseaudio>;

  class pulseaudio_module : public event_module<pulseaudio_module>, public input_handler {
   public:
    explicit pulseaudio_module(const bar_settings&, string);

    void teardown();
    bool has_event();
    bool update();
    string get_format() const;
    string get_output();
    bool build(builder* builder, const string& tag) const;

   protected:
    bool input(string&& cmd);

   private:
    static constexpr auto FORMAT_VOLUME = "format-volume";
    static constexpr auto FORMAT_MUTED = "format-muted";

    static constexpr auto TAG_RAMP_VOLUME = "<ramp-volume>";
    static constexpr auto TAG_BAR_VOLUME = "<bar-volume>";
    static constexpr auto TAG_LABEL_VOLUME = "<label-volume>";
    static constexpr auto TAG_LABEL_MUTED = "<label-muted>";

    static constexpr auto EVENT_PREFIX = "pa_vol";
    static constexpr auto EVENT_VOLUME_UP = "pa_volup";
    static constexpr auto EVENT_VOLUME_DOWN = "pa_voldown";
    static constexpr auto EVENT_TOGGLE_MUTE = "pa_volmute";

    progressbar_t m_bar_volume;
    ramp_t m_ramp_volume;
    label_t m_label_volume;
    label_t m_label_muted;

    pulseaudio_t m_pulseaudio;

    int m_interval{5};
    atomic<bool> m_muted{false};
    atomic<int> m_volume{0};
    atomic<double> m_decibels{0};
  };
}

POLYBAR_NS_END
