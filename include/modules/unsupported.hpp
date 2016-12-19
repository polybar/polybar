#pragma once
#if ENABLE_I3 && ENABLE_MPD && ENABLE_NETWORK && ENABLE_ALSA && ENABLE_CURL && WITH_XKB
#error "Support has been enabled for all optional modules"
#endif

#include "modules/meta/base.hpp"
#include "modules/meta/base.inl"

#if not(ENABLE_ALSA && ENABLE_I3 && ENABLE_MPD)
#include "modules/meta/event_module.inl"
#endif
#if not(ENABLE_NETWORK && ENABLE_CURL)
#include "modules/meta/timer_module.inl"
#endif
#if not WITH_XKB
#include "modules/meta/static_module.inl"
#endif

POLYBAR_NS

namespace modules {
  struct module_interface;

#define DEFINE_UNSUPPORTED_MODULE(MODULE_NAME, MODULE_TYPE)                             \
  class MODULE_NAME : public module_interface {                                         \
   public:                                                                              \
    MODULE_NAME(const bar_settings, string) {             \
      throw application_error("No built-in support for '" + string{MODULE_TYPE} + "'"); \
    }                                                                                   \
    string name() const {                                                               \
      return "";                                                                        \
    }                                                                                   \
    bool running() const {                                                              \
      return false;                                                                     \
    }                                                                                   \
    void setup() {}                                                                     \
    void start() {}                                                                     \
    void stop() {}                                                                      \
    void halt(string) {}                                                                \
    string contents() {                                                                 \
      return "";                                                                        \
    }                                                                                   \
    bool handle_event(string) {                                                         \
      return false;                                                                     \
    }                                                                                   \
    bool receive_events() const {                                                       \
      return false;                                                                     \
    }                                                                                   \
    void set_update_cb(callback<>&&) {}                                                 \
    void set_stop_cb(callback<>&&) {}                                                   \
  }

#if not ENABLE_I3
  DEFINE_UNSUPPORTED_MODULE(i3_module, "internal/i3");
#endif
#if not ENABLE_MPD
  DEFINE_UNSUPPORTED_MODULE(mpd_module, "internal/mpd");
#endif
#if not ENABLE_NETWORK
  DEFINE_UNSUPPORTED_MODULE(network_module, "internal/network");
#endif
#if not ENABLE_ALSA
  DEFINE_UNSUPPORTED_MODULE(volume_module, "internal/volume");
#endif
#if not ENABLE_CURL
  DEFINE_UNSUPPORTED_MODULE(github_module, "internal/github");
#endif
#if not WITH_XKB
  DEFINE_UNSUPPORTED_MODULE(xkeyboard_module, "internal/xkeyboard");
#endif
}

POLYBAR_NS_END
