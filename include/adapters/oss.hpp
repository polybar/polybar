#pragma once

#if defined __OpenBSD__
#  include <soundcard.h>
#else
#  include <sys/soundcard.h>
#endif

#include "common.hpp"
#include "settings.hpp"

POLYBAR_NS

namespace oss {
  DEFINE_ERROR(oss_exception);

  class mixer {
   public:
    explicit mixer(string&& mixer_device_path, string&& mixer_channel_name);
    ~mixer();

    mixer(const mixer& o) = delete;
    mixer& operator=(const mixer& o) = delete;

    const string& get_name();
    const string& get_sound_card();

    bool wait(int timeout = -1);

    int get_volume();
    void set_volume(float percentage);
    void set_mute(bool mode);
    void toggle_mute();
    bool is_muted();

   private:
    string m_device_path;
    int m_fd{-1};
    int m_channel{SOUND_MIXER_NONE};
    int m_stereodevs;
  };
}

POLYBAR_NS_END
