#pragma once

#include "modules/meta/base.hpp"
#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  struct module_interface;

#define DEFINE_UNSUPPORTED_MODULE(MODULE_NAME, MODULE_TYPE)                             \
  class MODULE_NAME : public module_interface {                                         \
   public:                                                                              \
    MODULE_NAME(const bar_settings, string) {                                           \
      throw application_error("No built-in support for '" + string{MODULE_TYPE} + "'"); \
    }                                                                                   \
    static constexpr auto TYPE = MODULE_TYPE;                                           \
    string type() const {                                                               \
      return "";                                                                        \
    }                                                                                   \
    string name_raw() const {                                                           \
      return "";                                                                        \
    }                                                                                   \
    string name() const {                                                               \
      return "";                                                                        \
    }                                                                                   \
    bool running() const {                                                              \
      return false;                                                                     \
    }                                                                                   \
    void start() {}                                                                     \
    void stop() {}                                                                      \
    void halt(string) {}                                                                \
    string contents() {                                                                 \
      return "";                                                                        \
    }                                                                                   \
    bool input(const string&, const string&) {                                          \
      return false;                                                                     \
    }                                                                                   \
  }

#if not ENABLE_I3
  DEFINE_UNSUPPORTED_MODULE(i3_module, "internal/i3");
#endif
#if not ENABLE_DWM
  DEFINE_UNSUPPORTED_MODULE(dwm_module, "internal/dwm");
#endif
#if not ENABLE_MPD
  DEFINE_UNSUPPORTED_MODULE(mpd_module, "internal/mpd");
#endif
#if not ENABLE_NETWORK
  DEFINE_UNSUPPORTED_MODULE(network_module, "internal/network");
#endif
#if not ENABLE_ALSA
  DEFINE_UNSUPPORTED_MODULE(alsa_module, "internal/alsa");
#endif
#if not ENABLE_PULSEAUDIO
  DEFINE_UNSUPPORTED_MODULE(pulseaudio_module, "internal/pulseaudio");
#endif
#if not ENABLE_CURL
  DEFINE_UNSUPPORTED_MODULE(github_module, "internal/github");
#endif
#if not ENABLE_XKEYBOARD
  DEFINE_UNSUPPORTED_MODULE(xkeyboard_module, "internal/xkeyboard");
#endif
}  // namespace modules

POLYBAR_NS_END
