#pragma once

#include <iostream>
#include <vector>

#include "version.hpp"

#define APP_NAME "@PROJECT_NAME@"
#cmakedefine APP_VERSION "@APP_VERSION@"
#ifndef APP_VERSION
#define APP_VERSION GIT_TAG
#endif
#define BASE_PATH "@PROJECT_SOURCE_DIR@"

#cmakedefine01 ENABLE_ALSA
#cmakedefine01 ENABLE_MPD
#cmakedefine01 ENABLE_NETWORK
#cmakedefine01 ENABLE_I3

#cmakedefine01 WITH_XRANDR
#cmakedefine01 WITH_XRENDER
#cmakedefine01 WITH_XDAMAGE
#cmakedefine01 WITH_XSYNC
#cmakedefine01 WITH_XCOMPOSITE
#cmakedefine01 WITH_XKB
#cmakedefine XPP_EXTENSION_LIST @XPP_EXTENSION_LIST@

#cmakedefine DEBUG_LOGGER
#cmakedefine VERBOSE_TRACELOG
#cmakedefine DEBUG_HINTS

#ifdef DEBUG_HINTS
static const int DEBUG_HINTS_OFFSET_X{@DEBUG_HINTS_OFFSET_X@};
static const int DEBUG_HINTS_OFFSET_Y{@DEBUG_HINTS_OFFSET_Y@};
#endif

static constexpr const char* ALSA_SOUNDCARD{"@SETTING_ALSA_SOUNDCARD@"};
static constexpr const char* BSPWM_SOCKET_PATH{"@SETTING_BSPWM_SOCKET_PATH@"};
static constexpr const char* BSPWM_STATUS_PREFIX{"@SETTING_BSPWM_STATUS_PREFIX@"};
static constexpr const char* CONNECTION_TEST_IP{"@SETTING_CONNECTION_TEST_IP@"};
static constexpr const char* PATH_ADAPTER{"@SETTING_PATH_ADAPTER@"};
static constexpr const char* PATH_BACKLIGHT_MAX{"@SETTING_PATH_BACKLIGHT_MAX@"};
static constexpr const char* PATH_BACKLIGHT_VAL{"@SETTING_PATH_BACKLIGHT_VAL@"};
static constexpr const char* PATH_BATTERY{"@SETTING_PATH_BATTERY@"};
static constexpr const char* PATH_CPU_INFO{"@SETTING_PATH_CPU_INFO@"};
static constexpr const char* PATH_MEMORY_INFO{"@SETTING_PATH_MEMORY_INFO@"};
static constexpr const char* PATH_MESSAGING_FIFO{"@SETTING_PATH_MESSAGING_FIFO@"};
static constexpr const char* PATH_TEMPERATURE_INFO{"@SETTING_PATH_TEMPERATURE_INFO@"};

static const int8_t DEFAULT_FONT_INDEX{-1};
static constexpr const char* BUILDER_SPACE_TOKEN{"%__"};

auto version_details = [](const std::vector<std::string>& args) {
  for (auto&& arg : args) {
  if (arg.compare(0, 3, "-vv") == 0)
      return true;
  }
  return false;
};

// clang-format off
auto print_build_info = [](bool extended = false) {
  std::cout << APP_NAME << " " << APP_VERSION << " " << "\n"
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
