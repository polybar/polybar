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

static constexpr const char* ALSA_SOUNDCARD{"default"};
static constexpr const char* BSPWM_SOCKET_PATH{"/tmp/bspwm_0_0-socket"};
static constexpr const char* BSPWM_STATUS_PREFIX{"W"};
static constexpr const char* CONNECTION_TEST_IP{"8.8.8.8"};
static constexpr const char* PATH_ADAPTER{"/sys/class/power_supply/%adapter%"};
static constexpr const char* PATH_BACKLIGHT_MAX{"/sys/class/backlight/%card%/max_brightness"};
static constexpr const char* PATH_BACKLIGHT_VAL{"/sys/class/backlight/%card%/brightness"};
static constexpr const char* PATH_BATTERY{"/sys/class/power_supply/%battery%"};
static constexpr const char* PATH_CPU_INFO{"/proc/stat"};
static constexpr const char* PATH_MEMORY_INFO{"/proc/meminfo"};
static constexpr const char* PATH_MESSAGING_FIFO{"/tmp/polybar_mqueue.%pid%"};
static constexpr const char* PATH_TEMPERATURE_INFO{"/sys/class/thermal/thermal_zone%zone%/temp"};

static constexpr const char* BUILDER_SPACE_TOKEN{"%__"};

static const int8_t DEFAULT_FONT_INDEX{-1};

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
            << "PATH_ADAPTER                " << PATH_ADAPTER               << "\n"
            << "PATH_BACKLIGHT_MAX          " << PATH_BACKLIGHT_MAX         << "\n"
            << "PATH_BACKLIGHT_VAL          " << PATH_BACKLIGHT_VAL         << "\n"
            << "PATH_BATTERY                " << PATH_BATTERY               << "\n"
            << "PATH_CPU_INFO               " << PATH_CPU_INFO              << "\n"
            << "PATH_MEMORY_INFO            " << PATH_MEMORY_INFO           << "\n"
            << "PATH_TEMPERATURE_INFO       " << PATH_TEMPERATURE_INFO      << "\n";
};
// clang-format on

// vim:ft=cpp
