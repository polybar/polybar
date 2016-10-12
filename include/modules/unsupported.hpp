#pragma once

#include "modules/meta.hpp"

#define DEFINE_UNSUPPORTED_MODULE(MODULE_NAME, MODULE_TYPE)                             \
  class MODULE_NAME : public module<MODULE_NAME> {                                      \
   public:                                                                              \
    using module<MODULE_NAME>::module;                                                  \
    MODULE_NAME(const bar_settings b, const logger& l, const config& c, string n)       \
        : module<MODULE_NAME>::module(b, l, c, n) {                                     \
      throw application_error("No built-in support for '" + string{MODULE_TYPE} + "'"); \
    }                                                                                   \
    void start() {}                                                                     \
    bool build(builder*, string) {                                                      \
      return true;                                                                      \
    }                                                                                   \
  }

LEMONBUDDY_NS

namespace modules {
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
}

LEMONBUDDY_NS_END
