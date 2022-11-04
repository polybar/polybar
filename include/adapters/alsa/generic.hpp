#pragma once

#ifdef USE_ALSALIB_H
#include <alsa/asoundlib.h>
#else
#include <assert.h>

#ifndef __FreeBSD__
#include <endian.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef __GNUC__
#define __inline__ inline
#endif

#include <alsa/asoundef.h>
#include <alsa/conf.h>
#include <alsa/control.h>
#include <alsa/error.h>
#include <alsa/global.h>
#include <alsa/hwdep.h>
#include <alsa/input.h>
#include <alsa/mixer.h>
#include <alsa/output.h>
#include <alsa/pcm.h>
#include <alsa/rawmidi.h>
#include <alsa/seq.h>
#include <alsa/seq_event.h>
#include <alsa/seq_midi_event.h>
#include <alsa/seqmid.h>
#include <alsa/timer.h>
#include <alsa/version.h>
#endif

#include "common.hpp"
#include "errors.hpp"
#include "settings.hpp"

POLYBAR_NS

namespace alsa {
  DEFINE_ERROR(alsa_exception);
  DEFINE_CHILD_ERROR(mixer_error, alsa_exception);
  DEFINE_CHILD_ERROR(control_error, alsa_exception);

  template <typename T>
  void throw_exception(string&& message, int error_code) {
    const char* snd_error = snd_strerror(error_code);
    if (snd_error != nullptr)
      message += ": " + string{snd_error};
    throw T(message.c_str());
  }
} // namespace alsa

POLYBAR_NS_END
