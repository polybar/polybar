#pragma once

#include <functional>
#include <string>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <mutex>

#include "exception.hpp"

#define STRSNDERR(s) std::string(snd_strerror(s))

namespace alsa
{
  class Exception : public ::Exception
  {
    public:
      explicit Exception(const std::string& msg) : ::Exception("[Alsa] "+ msg){}
  };

  class ControlInterfaceError : public Exception
  {
    public:
      ControlInterfaceError(int code, const std::string& msg)
        : Exception(msg +" ["+ std::to_string(code) +"]") {}
  };

  class ControlInterface
  {
    std::mutex mtx;

    snd_hctl_t *hctl;
    snd_hctl_elem_t *elem;

    snd_ctl_t *ctl;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_value_t *value;
    snd_ctl_elem_id_t *id;

    public:
      explicit ControlInterface(int numid);
      ~ControlInterface();

      bool wait(int timeout = -1);

      bool test_device_plugged();
  };


  class MixerError : public Exception {
    using Exception::Exception;
  };

  class Mixer
  {
    std::mutex mtx;

    snd_mixer_t *hardware_mixer = nullptr;
    snd_mixer_elem_t *mixer_element = nullptr;

    public:
      explicit Mixer(const std::string& mixer_control_name);
      ~Mixer();

      bool wait(int timeout = -1);

      int get_volume();
      void set_volume(float percentage);
      void set_mute(bool mode);
      void toggle_mute();
      bool is_muted();

    protected:
      void error_handler(const std::string& message);
  };
}
