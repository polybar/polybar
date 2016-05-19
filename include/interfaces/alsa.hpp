#ifndef _INTERFACES_ALSA_HPP_
#define _INTERFACES_ALSA_HPP_

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
      Exception(const std::string& msg) : ::Exception("[Alsa] "+ msg){}
  };

  class ControlInterfaceError : public Exception {
    using Exception::Exception;
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
      ControlInterface(int numid) throw(ControlInterfaceError);
      ~ControlInterface();

      bool wait(int timeout = -1) throw(ControlInterfaceError);

      bool test_device_plugged() throw(ControlInterfaceError);
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
      Mixer(const std::string& mixer_control_name) throw(MixerError);
      ~Mixer();

      bool wait(int timeout = -1) throw(MixerError);

      int get_volume();
      void set_volume(float percentage);
      void set_mute(bool mode);
      void toggle_mute();
      bool is_muted();

    protected:
      void error_handler(const std::string& message);
  };
}

#endif
