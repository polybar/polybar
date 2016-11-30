#pragma once

#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef __GNUC__
#define __inline__ inline
#endif

#include <alsa/asoundef.h>
#include <alsa/version.h>
#include <alsa/global.h>
#include <alsa/input.h>
#include <alsa/output.h>
#include <alsa/error.h>
#include <alsa/conf.h>
#include <alsa/pcm.h>
#include <alsa/rawmidi.h>
#include <alsa/timer.h>
#include <alsa/hwdep.h>
#include <alsa/control.h>
#include <alsa/mixer.h>
#include <alsa/seq_event.h>
#include <alsa/seq.h>
#include <alsa/seqmid.h>
#include <alsa/seq_midi_event.h>

#include <cmath>
#include <functional>
#include <string>

#include "common.hpp"
#include "config.hpp"
#include "errors.hpp"
#include "utils/concurrency.hpp"

#define MAX_LINEAR_DB_SCALE 24

POLYBAR_NS

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
  int m_numid{0};

  std::mutex m_lock;

  snd_hctl_t* m_hctl{nullptr};
  snd_hctl_elem_t* m_elem{nullptr};

  snd_ctl_t* m_ctl{nullptr};
  snd_ctl_elem_info_t* m_info{nullptr};
  snd_ctl_elem_value_t* m_value{nullptr};
  snd_ctl_elem_id_t* m_id{nullptr};
};

// }}}
// class definition : alsa_mixer {{{

class alsa_mixer {
 public:
  explicit alsa_mixer(const string& mixer_control_name);
  ~alsa_mixer();

  string get_name();

  bool wait(int timeout = -1);
  int process_events();

  int get_volume();
  int get_normalized_volume();
  void set_volume(float percentage);
  void set_normalized_volume(float percentage);
  void set_mute(bool mode);
  void toggle_mute();
  bool is_muted();

 private:
  string m_name;

  std::mutex m_lock;

  snd_mixer_t* m_hardwaremixer{nullptr};
  snd_mixer_elem_t* m_mixerelement{nullptr};
};

// }}}

POLYBAR_NS_END
