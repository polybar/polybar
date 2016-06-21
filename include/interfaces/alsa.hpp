#pragma once

#include <functional>
#include <string>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <mutex>

#include "exception.hpp"
#include "utils/concurrency.hpp"
#include "utils/macros.hpp"

#define StrSndErr(s) ToStr(snd_strerror(s))

namespace alsa
{
  // Errors {{{

  class Exception : public ::Exception
  {
    public:
      explicit Exception(std::string msg) : ::Exception("[Alsa] "+ msg){}
  };

  class ControlInterfaceError : public Exception
  {
    public:
      ControlInterfaceError(int code, std::string msg)
        : Exception(msg +" ["+ std::to_string(code) +"]") {}
  };

  class MixerError : public Exception {
    using Exception::Exception;
  };

  // }}}
  // ControlInterface {{{

  class ControlInterface
  {
    concurrency::SpinLock lock;

    snd_hctl_t *hctl;
    snd_hctl_elem_t *elem;

    snd_ctl_t *ctl;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_value_t *value;
    snd_ctl_elem_id_t *id;

    public:
      explicit ControlInterface(int numid);
      ~ControlInterface();
      ControlInterface(const ControlInterface &) = delete;
      ControlInterface &operator=(const ControlInterface &) = delete;

      bool wait(int timeout = -1);
      void process_events();

      bool test_device_plugged();
  };

  // }}}
  // Mixer {{{

  class Mixer
  {
    concurrency::SpinLock lock;

    snd_mixer_t *hardware_mixer = nullptr;
    snd_mixer_elem_t *mixer_element = nullptr;

    public:
      explicit Mixer(std::string mixer_control_name);
      ~Mixer();
      Mixer(const Mixer &) = delete;
      Mixer &operator=(const Mixer &) = delete;

      bool wait(int timeout = -1);
      int process_events();

      int get_volume();
      void set_volume(float percentage);
      void set_mute(bool mode);
      void toggle_mute();
      bool is_muted();

    protected:
      void error_handler(std::string message);
  };

  // }}}
}
