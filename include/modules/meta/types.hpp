#pragma once

#include "common.hpp"

POLYBAR_NS

namespace modules {

  static constexpr auto ALSA_TYPE = "internal/alsa";
  static constexpr auto BACKLIGHT_TYPE = "internal/backlight";
  static constexpr auto BATTERY_TYPE = "internal/battery";
  static constexpr auto BSPWM_TYPE = "internal/bspwm";
  static constexpr auto COUNTER_TYPE = "internal/counter";
  static constexpr auto CPU_TYPE = "internal/cpu";
  static constexpr auto DATE_TYPE = "internal/date";
  static constexpr auto FS_TYPE = "internal/fs";
  static constexpr auto GITHUB_TYPE = "internal/github";
  static constexpr auto I3_TYPE = "internal/i3";
  static constexpr auto MEMORY_TYPE = "internal/memory";
  static constexpr auto MPD_TYPE = "internal/mpd";
  static constexpr auto NETWORK_TYPE = "internal/network";
  static constexpr auto PULSEAUDIO_TYPE = "internal/pulseaudio";
  static constexpr auto TEMPERATURE_TYPE = "internal/temperature";
  static constexpr auto TRAY_TYPE = "internal/tray";
  static constexpr auto XBACKLIGHT_TYPE = "internal/xbacklight";
  static constexpr auto XKEYBOARD_TYPE = "internal/xkeyboard";
  static constexpr auto XWINDOW_TYPE = "internal/xwindow";
  static constexpr auto XWORKSPACES_TYPE = "internal/xworkspaces";

  static constexpr auto IPC_TYPE = "custom/ipc";
  static constexpr auto MENU_TYPE = "custom/menu";
  static constexpr auto SCRIPT_TYPE = "custom/script";
  static constexpr auto TEXT_TYPE = "custom/text";

} // namespace modules

POLYBAR_NS_END