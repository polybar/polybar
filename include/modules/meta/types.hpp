#pragma once

#include "common.hpp"

POLYBAR_NS

namespace modules {

  static constexpr const char ALSA_TYPE[] = "internal/alsa";
  static constexpr const char BACKLIGHT_TYPE[] = "internal/backlight";
  static constexpr const char BATTERY_TYPE[] = "internal/battery";
  static constexpr const char BSPWM_TYPE[] = "internal/bspwm";
  static constexpr const char COUNTER_TYPE[] = "internal/counter";
  static constexpr const char CPU_TYPE[] = "internal/cpu";
  static constexpr const char DATE_TYPE[] = "internal/date";
  static constexpr const char FS_TYPE[] = "internal/fs";
  static constexpr const char GITHUB_TYPE[] = "internal/github";
  static constexpr const char I3_TYPE[] = "internal/i3";
  static constexpr const char MEMORY_TYPE[] = "internal/memory";
  static constexpr const char MPD_TYPE[] = "internal/mpd";
  static constexpr const char NETWORK_TYPE[] = "internal/network";
  static constexpr const char PULSEAUDIO_TYPE[] = "internal/pulseaudio";
  static constexpr const char TEMPERATURE_TYPE[] = "internal/temperature";
  static constexpr const char TRAY_TYPE[] = "internal/tray";
  static constexpr const char XBACKLIGHT_TYPE[] = "internal/xbacklight";
  static constexpr const char XKEYBOARD_TYPE[] = "internal/xkeyboard";
  static constexpr const char XWINDOW_TYPE[] = "internal/xwindow";
  static constexpr const char XWORKSPACES_TYPE[] = "internal/xworkspaces";

  static constexpr const char IPC_TYPE[] = "custom/ipc";
  static constexpr const char MENU_TYPE[] = "custom/menu";
  static constexpr const char SCRIPT_TYPE[] = "custom/script";
  static constexpr const char TEXT_TYPE[] = "custom/text";

} // namespace modules

POLYBAR_NS_END