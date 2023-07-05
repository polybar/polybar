#pragma once

#include "modules/meta/event_module.hpp"
#include "modules/meta/types.hpp"
#include "settings.hpp"

POLYBAR_NS

// fwd
namespace alsa {
  class mixer;
  class control;
}  // namespace alsa

namespace modules {
  enum class mixer { NONE = 0, MASTER, SPEAKER, HEADPHONE };
  enum class control { NONE = 0, HEADPHONE };

  using mixer_t = shared_ptr<alsa::mixer>;
  using control_t = shared_ptr<alsa::control>;

  class alsa_module : public event_module<alsa_module> {
   public:
    explicit alsa_module(const bar_settings&, string, const config&);

    void teardown();
    bool has_event();
    bool update();
    string get_format() const;
    string get_output();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = ALSA_TYPE;

    static constexpr auto EVENT_INC = "inc";
    static constexpr auto EVENT_DEC = "dec";
    static constexpr auto EVENT_TOGGLE = "toggle";

   protected:
    void action_inc();
    void action_dec();
    void action_toggle();

    void change_volume(int interval);

    void action_epilogue(const vector<mixer_t>& mixers);

    vector<mixer_t> get_mixers();

   private:
    static constexpr auto FORMAT_VOLUME = "format-volume";
    static constexpr auto FORMAT_MUTED = "format-muted";

#define DEF_RAMP_VOLUME "ramp-volume"
#define DEF_RAMP_HEADPHONES "ramp-headphones"
#define DEF_BAR_VOLUME "bar-volume"
#define DEF_LABEL_VOLUME "label-volume"
#define DEF_LABEL_MUTED "label-muted"

    static constexpr auto NAME_RAMP_VOLUME = DEF_RAMP_VOLUME;
    static constexpr auto NAME_RAMP_HEADPHONES = DEF_RAMP_HEADPHONES;
    static constexpr auto NAME_BAR_VOLUME = DEF_BAR_VOLUME;
    static constexpr auto NAME_LABEL_VOLUME = DEF_LABEL_VOLUME;
    static constexpr auto NAME_LABEL_MUTED = DEF_LABEL_MUTED;

    static constexpr auto TAG_RAMP_VOLUME = "<" DEF_RAMP_VOLUME ">";
    static constexpr auto TAG_RAMP_HEADPHONES = "<" DEF_RAMP_HEADPHONES ">";
    static constexpr auto TAG_BAR_VOLUME = "<" DEF_BAR_VOLUME ">";
    static constexpr auto TAG_LABEL_VOLUME = "<" DEF_LABEL_VOLUME ">";
    static constexpr auto TAG_LABEL_MUTED = "<" DEF_LABEL_MUTED ">";

#undef DEF_RAMP_VOLUME
#undef DEF_RAMP_HEADPHONES
#undef DEF_BAR_VOLUME
#undef DEF_LABEL_VOLUME
#undef DEF_LABEL_MUTED

    progressbar_t m_bar_volume;
    ramp_t m_ramp_volume;
    ramp_t m_ramp_headphones;
    label_t m_label_volume;
    label_t m_label_muted;

    map<mixer, mixer_t> m_mixer;
    map<control, control_t> m_ctrl;
    int m_headphoneid{0};
    bool m_mapped{false};
    int m_interval{5};
    atomic<bool> m_muted{false};
    atomic<bool> m_headphones{false};
    atomic<int> m_volume{0};
  };
}  // namespace modules

POLYBAR_NS_END
