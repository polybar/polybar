#pragma once

#include "config.hpp"
#include "modules/meta/event_module.hpp"

POLYBAR_NS

// fwd
namespace alsa {
  class mixer;
  class control;
}

namespace modules {
  enum class mixer { NONE = 0, MASTER, SPEAKER, HEADPHONE };
  enum class control { NONE = 0, HEADPHONE };

  using mixer_t = shared_ptr<alsa::mixer>;
  using control_t = shared_ptr<alsa::control>;

  class volume_module : public event_module<volume_module> {
   public:
    explicit volume_module(const bar_settings&, string);

    void teardown();
    bool has_event();
    bool update();
    string get_format() const;
    string get_output();
    bool build(builder* builder, const string& tag) const;
    bool handle_event(string cmd);
    bool receive_events() const;

   private:
    static constexpr auto FORMAT_VOLUME = "format-volume";
    static constexpr auto FORMAT_MUTED = "format-muted";

    static constexpr auto TAG_RAMP_VOLUME = "<ramp-volume>";
    static constexpr auto TAG_RAMP_HEADPHONES = "<ramp-headphones>";
    static constexpr auto TAG_BAR_VOLUME = "<bar-volume>";
    static constexpr auto TAG_LABEL_VOLUME = "<label-volume>";
    static constexpr auto TAG_LABEL_MUTED = "<label-muted>";

    static constexpr auto EVENT_PREFIX = "vol";
    static constexpr auto EVENT_VOLUME_UP = "volup";
    static constexpr auto EVENT_VOLUME_DOWN = "voldown";
    static constexpr auto EVENT_TOGGLE_MUTE = "volmute";

    progressbar_t m_bar_volume;
    ramp_t m_ramp_volume;
    ramp_t m_ramp_headphones;
    label_t m_label_volume;
    label_t m_label_muted;

    map<mixer, mixer_t> m_mixer;
    map<control, control_t> m_ctrl;
    int m_headphoneid{0};
    bool m_mapped{false};
    stateflag m_muted{false};
    stateflag m_headphones{false};
    std::atomic<int> m_volume{0};
  };
}

POLYBAR_NS_END
