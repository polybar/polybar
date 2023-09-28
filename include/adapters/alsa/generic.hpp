#pragma once

#include <alsa/asoundlib.h>

#include "common.hpp"
#include "settings.hpp"
#include "errors.hpp"

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
}

POLYBAR_NS_END
