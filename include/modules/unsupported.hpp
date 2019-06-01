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
    string type() const override {                                                      \
      return "";                                                                        \
    }                                                                                   \
    string name_raw() const override {                                                  \
      return "";                                                                        \
    }                                                                                   \
    string name() const override {                                                      \
      return "";                                                                        \
    }                                                                                   \
    bool running() const override {                                                     \
      return false;                                                                     \
    }                                                                                   \
    bool visible() const override {                                                     \
      return false;                                                                     \
    }                                                                                   \
    void start() override {}                                                            \
    void stop() override {}                                                             \
    void join() override {}                                                             \
    void halt(string) override {}                                                       \
    string contents() override {                                                        \
      return "";                                                                        \
    }                                                                                   \
    bool input(const string&, const string&) override {                                 \
      return false;                                                                     \
    }                                                                                   \
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
