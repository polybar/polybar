#pragma once

#include <iostream>

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
#cmakedefine DRAW_CLICKABLE_AREA_HINTS_OFFSET_Y @DRAW_CLICKABLE_AREA_HINTS_OFFSET_Y@
#ifndef DRAW_CLICKABLE_AREA_HINTS_OFFSET_Y
#define DRAW_CLICKABLE_AREA_HINTS_OFFSET_Y 0
#endif
#endif

#define BUILDER_SPACE_TOKEN "%__"
#define ALSA_SOUNDCARD "@SETTING_ALSA_SOUNDCARD@"
#define CONNECTION_TEST_IP "@SETTING_CONNECTION_TEST_IP@"
#define PATH_BACKLIGHT_VAL "@SETTING_PATH_BACKLIGHT_VAL@"
#define PATH_BACKLIGHT_MAX "@SETTING_PATH_BACKLIGHT_MAX@"
#define PATH_BATTERY_CAPACITY "@SETTING_PATH_BATTERY_CAPACITY@"
#define PATH_ADAPTER_STATUS "@SETTING_PATH_ADAPTER_STATUS@"
#define BSPWM_SOCKET_PATH "@SETTING_BSPWM_SOCKET_PATH@"
#define BSPWM_STATUS_PREFIX "@SETTING_BSPWM_STATUS_PREFIX@"
#define PATH_CPU_INFO "@SETTING_PATH_CPU_INFO@"
#define PATH_MEMORY_INFO "@SETTING_PATH_MEMORY_INFO@"

auto print_build_info = []() {
  // clang-format off
  std::cout << APP_NAME << " " << GIT_TAG
            << "\n\n"
            << "Built with: "
              << (ENABLE_ALSA     ? "+" : "-") << "alsa "
              << (ENABLE_I3       ? "+" : "-") << "i3 "
              << (ENABLE_MPD      ? "+" : "-") << "mpd "
              << (ENABLE_NETWORK  ? "+" : "-") << "network "
            << "\n\n"
            << "ALSA_SOUNDCARD        " << ALSA_SOUNDCARD        << "\n"
            << "BSPWM_SOCKET_PATH     " << BSPWM_SOCKET_PATH     << "\n"
            << "BSPWM_STATUS_PREFIX   " << BSPWM_STATUS_PREFIX   << "\n"
            << "BUILDER_SPACE_TOKEN   " << BUILDER_SPACE_TOKEN   << "\n"
            << "CONNECTION_TEST_IP    " << CONNECTION_TEST_IP    << "\n"
            << "PATH_ADAPTER_STATUS   " << PATH_ADAPTER_STATUS   << "\n"
            << "PATH_BACKLIGHT_MAX    " << PATH_BACKLIGHT_MAX    << "\n"
            << "PATH_BACKLIGHT_VAL    " << PATH_BACKLIGHT_VAL    << "\n"
            << "PATH_BATTERY_CAPACITY " << PATH_BATTERY_CAPACITY << "\n"
            << "PATH_CPU_INFO         " << PATH_CPU_INFO         << "\n"
            << "PATH_MEMORY_INFO      " << PATH_MEMORY_INFO      << "\n";
  // clang-format on
};

// vim:ft=cpp
