#pragma once

#include <iostream>
#include <vector>

#include "version.hpp"

#define APP_NAME "@PROJECT_NAME@"
#define BASE_PATH "@PROJECT_SOURCE_DIR@"

#cmakedefine01 ENABLE_ALSA
#cmakedefine01 ENABLE_MPD
#cmakedefine01 ENABLE_NETWORK
#cmakedefine01 ENABLE_I3

#cmakedefine ENABLE_RANDR_EXT
#cmakedefine ENABLE_RENDER_EXT
#cmakedefine ENABLE_DAMAGE_EXT

#cmakedefine DEBUG_LOGGER
#cmakedefine VERBOSE_TRACELOG

#ifdef DEBUG
#cmakedefine01 DRAW_CLICKABLE_AREA_HINTS
#cmakedefine DRAW_CLICKABLE_AREA_HINTS_OFFSET_Y @DRAW_CLICKABLE_AREA_HINTS_OFFSET_Y @
#ifndef DRAW_CLICKABLE_AREA_HINTS_OFFSET_Y
#define DRAW_CLICKABLE_AREA_HINTS_OFFSET_Y 0
#endif
#endif

#define BUILDER_SPACE_TOKEN "%__"

#define ALSA_SOUNDCARD "@SETTING_ALSA_SOUNDCARD@"
#define BSPWM_SOCKET_PATH "@SETTING_BSPWM_SOCKET_PATH@"
#define BSPWM_STATUS_PREFIX "@SETTING_BSPWM_STATUS_PREFIX@"
#define CONNECTION_TEST_IP "@SETTING_CONNECTION_TEST_IP@"
#define PATH_ADAPTER_STATUS "@SETTING_PATH_ADAPTER_STATUS@"
#define PATH_BACKLIGHT_MAX "@SETTING_PATH_BACKLIGHT_MAX@"
#define PATH_BACKLIGHT_VAL "@SETTING_PATH_BACKLIGHT_VAL@"
#define PATH_BATTERY_CAPACITY "@SETTING_PATH_BATTERY_CAPACITY@"
#define PATH_BATTERY_CAPACITY_MAX "@SETTING_PATH_BATTERY_CAPACITY_MAX@"
#define PATH_BATTERY_CAPACITY_PERC "@SETTING_PATH_BATTERY_CAPACITY_PERC@"
#define PATH_BATTERY_RATE "@SETTING_PATH_BATTERY_RATE@"
#define PATH_BATTERY_VOLTAGE "@SETTING_PATH_BATTERY_VOLTAGE@"
#define PATH_CPU_INFO "@SETTING_PATH_CPU_INFO@"
#define PATH_MEMORY_INFO "@SETTING_PATH_MEMORY_INFO@"
#define PATH_MESSAGING_FIFO "@SETTING_PATH_MESSAGING_FIFO@"
#define PATH_TEMPERATURE_INFO "@SETTING_PATH_TEMPERATURE_INFO@"

auto version_details = [](const std::vector<std::string>& args) {
  for (auto&& arg : args) {
  if (arg.compare(0, 3, "-vv") == 0)
      return true;
  }
  return false;
};

// clang-format off
auto print_build_info = [](bool extended = false) {
  std::cout << APP_NAME << " " << GIT_TAG << " " << "\n"
            << "\n"
            << "Features: "
            << (ENABLE_ALSA    ? "+" : "-") << "alsa "
            << (ENABLE_I3      ? "+" : "-") << "i3 "
            << (ENABLE_MPD     ? "+" : "-") << "mpd "
            << (ENABLE_NETWORK ? "+" : "-") << "network "
            << "\n";
  if (!extended)
    return;
  std::cout << "\n"
            << "Build type: @CMAKE_BUILD_TYPE@" << "\n"
            << "Compiler flags: @CMAKE_CXX_FLAGS@" << "\n"
            << "Linker flags: @CMAKE_EXE_LINKER_FLAGS@" << "\n"
            << "\n"
            << "ALSA_SOUNDCARD              " << ALSA_SOUNDCARD             << "\n"
            << "BSPWM_SOCKET_PATH           " << BSPWM_SOCKET_PATH          << "\n"
            << "BSPWM_STATUS_PREFIX         " << BSPWM_STATUS_PREFIX        << "\n"
            << "BUILDER_SPACE_TOKEN         " << BUILDER_SPACE_TOKEN        << "\n"
            << "CONNECTION_TEST_IP          " << CONNECTION_TEST_IP         << "\n"
            << "PATH_ADAPTER_STATUS         " << PATH_ADAPTER_STATUS        << "\n"
            << "PATH_BACKLIGHT_MAX          " << PATH_BACKLIGHT_MAX         << "\n"
            << "PATH_BACKLIGHT_VAL          " << PATH_BACKLIGHT_VAL         << "\n"
            << "PATH_BATTERY_CAPACITY       " << PATH_BATTERY_CAPACITY      << "\n"
            << "PATH_BATTERY_CAPACITY       " << PATH_BATTERY_CAPACITY      << "\n"
            << "PATH_BATTERY_CAPACITY_MAX   " << PATH_BATTERY_CAPACITY_MAX  << "\n"
            << "PATH_BATTERY_CAPACITY_PERC  " << PATH_BATTERY_CAPACITY_PERC << "\n"
            << "PATH_BATTERY_RATE           " << PATH_BATTERY_RATE          << "\n"
            << "PATH_BATTERY_VOLTAGE        " << PATH_BATTERY_VOLTAGE       << "\n"
            << "PATH_CPU_INFO               " << PATH_CPU_INFO              << "\n"
            << "PATH_MEMORY_INFO            " << PATH_MEMORY_INFO           << "\n"
            << "PATH_TEMPERATURE_INFO       " << PATH_TEMPERATURE_INFO      << "\n";
};
// clang-format on

// vim:ft=cpp
