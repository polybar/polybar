#pragma once

#include <array>
#include "common.hpp"
#include "components/logger.hpp"

POLYBAR_NS

class plugin_handle {
 public:
  plugin_handle(const char* libname);
  ~plugin_handle();

  plugin_handle(const plugin_handle&) = delete;
  plugin_handle& operator=(const plugin_handle&) = delete;

  plugin_handle(plugin_handle&&);
  plugin_handle& operator=(plugin_handle&&);

 private:
  void* m_handle = nullptr;
};

// clang-format off
const static std::array<const char*, 8> plugin_names = {
  "libpolybar-utils-i3.so",
  "libpolybar-modules-alsa.so",
  "libpolybar-modules-github.so",
  "libpolybar-modules-i3.so",
  "libpolybar-modules-mpd.so",
  "libpolybar-modules-network.so",
  "libpolybar-modules-pulseaudio.so",
  "libpolybar-modules-xkeyboard.so"
};
// clang-format on

POLYBAR_NS_END
