#pragma once

#include "modules/base.hpp"
#include "interfaces/alsa.hpp"
#include "drawtypes/icon.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/ramp.hpp"
#include "drawtypes/bar.hpp"

namespace modules
{
  DefineModule(VolumeModule, EventModule)
  {
    static constexpr auto FORMAT_VOLUME = "format:volume";
    static constexpr auto FORMAT_MUTED = "format:muted";

    static constexpr auto TAG_RAMP_VOLUME = "<ramp:volume>";
    static constexpr auto TAG_BAR_VOLUME = "<bar:volume>";
    static constexpr auto TAG_LABEL_VOLUME = "<label:volume>";
    static constexpr auto TAG_LABEL_MUTED = "<label:muted>";

    static constexpr auto EVENT_PREFIX = "vol";
    static constexpr auto EVENT_VOLUME_UP = "volup";
    static constexpr auto EVENT_VOLUME_DOWN = "voldown";
    static constexpr auto EVENT_TOGGLE_MUTE = "volmute";

    std::unique_ptr<drawtypes::Bar> bar_volume;
    std::unique_ptr<drawtypes::Ramp> ramp_volume;
    std::unique_ptr<drawtypes::Label> label_volume;
    std::unique_ptr<drawtypes::Label> label_volume_tokenized;
    std::unique_ptr<drawtypes::Label> label_muted;
    std::unique_ptr<drawtypes::Label> label_muted_tokenized;

    std::unique_ptr<alsa::Mixer> master_mixer;
    std::unique_ptr<alsa::Mixer> speaker_mixer;
    std::unique_ptr<alsa::Mixer> headphone_mixer;
    std::unique_ptr<alsa::ControlInterface> headphone_ctrl;

    int headphone_ctrl_numid;

    concurrency::Atomic<int> volume;
    concurrency::Atomic<bool> muted;
    concurrency::Atomic<bool> has_changed;

    public:
      explicit VolumeModule(const std::string& name);
      ~VolumeModule();

      bool has_event();
      bool update();

      std::string get_format();
      bool build(Builder *builder, const std::string& tag);

      std::string get_output();
      bool handle_command(const std::string& cmd);
  };
}
