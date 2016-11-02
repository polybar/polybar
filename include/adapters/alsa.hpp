#pragma once

#include <stdio.h>
#include <functional>
#include <string>

#include <alsa/asoundlib.h>

#include "common.hpp"
#include "config.hpp"
#include "utils/threading.hpp"

LEMONBUDDY_NS

DEFINE_ERROR(alsa_exception);
DEFINE_CHILD_ERROR(alsa_ctl_interface_error, alsa_exception);
DEFINE_CHILD_ERROR(alsa_mixer_error, alsa_exception);

// class definition : alsa_ctl_interface {{{

template <typename T>
void throw_exception(string&& message, int error_code) {
  const char* snd_error = snd_strerror(error_code);
  if (snd_error != nullptr)
    message += ": " + string{snd_error};
  throw T(message.c_str());
}

class alsa_ctl_interface {
 public:
  explicit alsa_ctl_interface(int numid);
  ~alsa_ctl_interface();

  int get_numid();
  bool wait(int timeout = -1);
  bool test_device_plugged();
  void process_events();

 private:
  int m_numid = 0;

  threading_util::spin_lock m_lock;

  snd_hctl_t* m_hctl = nullptr;
  snd_hctl_elem_t* m_elem = nullptr;

  snd_ctl_t* m_ctl = nullptr;
  snd_ctl_elem_info_t* m_info = nullptr;
  snd_ctl_elem_value_t* m_value = nullptr;
  snd_ctl_elem_id_t* m_id = nullptr;
};

// }}}
// class definition : alsa_mixer {{{

class alsa_mixer {
 public:
  explicit alsa_mixer(string mixer_control_name);
  ~alsa_mixer();

  string get_name();

  bool wait(int timeout = -1);
  int process_events();

  int get_volume();
  void set_volume(float percentage);
  void set_mute(bool mode);
  void toggle_mute();
  bool is_muted();

 private:
  string m_name;

  threading_util::spin_lock m_lock;

  snd_mixer_t* m_hardwaremixer = nullptr;
  snd_mixer_elem_t* m_mixerelement = nullptr;
};

// }}}

LEMONBUDDY_NS_END
