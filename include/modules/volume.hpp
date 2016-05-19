#ifndef _MODULES_VOLUME_HPP_
#define _MODULES_VOLUME_HPP_

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
    const char *FORMAT_VOLUME = "format:volume";
    const char *FORMAT_MUTED = "format:muted";

    const char *TAG_RAMP_VOLUME = "<ramp:volume>";
    const char *TAG_BAR_VOLUME = "<bar:volume>";
    const char *TAG_LABEL_VOLUME = "<label:volume>";
    const char *TAG_LABEL_MUTED = "<label:muted>";

    const char *EVENT_VOLUME_UP = "volup";
    const char *EVENT_VOLUME_DOWN = "voldown";
    const char *EVENT_TOGGLE_MUTE = "volmute";

    std::unique_ptr<alsa::Mixer> master_mixer;
    std::unique_ptr<alsa::Mixer> speaker_mixer;
    std::unique_ptr<alsa::Mixer> headphone_mixer;
    std::unique_ptr<alsa::ControlInterface> headphone_ctrl;
    int headphone_ctrl_numid;

    std::unique_ptr<Builder> builder;

    std::unique_ptr<drawtypes::Bar> bar_volume;
    std::unique_ptr<drawtypes::Ramp> ramp_volume;
    std::unique_ptr<drawtypes::Label> label_volume;
    std::unique_ptr<drawtypes::Label> label_volume_tokenized;
    std::unique_ptr<drawtypes::Label> label_muted;
    std::unique_ptr<drawtypes::Label> label_muted_tokenized;

    int volume = 0;
    bool muted = false;

    public:
      VolumeModule(const std::string& name) throw(ModuleError);
      ~VolumeModule();

      bool has_event();
      bool update();

      std::string get_format();
      bool build(Builder *builder, const std::string& tag);

      std::string get_output();
      bool handle_command(const std::string& cmd);
  };
}

#endif
