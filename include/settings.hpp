#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include "version.hpp"

#define APP_NAME "polybar"
#define APP_VERSION "2.4.8-9-gc1aa331-git"
#ifndef APP_VERSION
#define APP_VERSION GIT_TAG
#endif
#define BASE_PATH "/home/jaagr/.local/var/github/jaagr/polybar"

#define ENABLE_ALSA 1
#define ENABLE_MPD 1
#define ENABLE_NETWORK 1
#define ENABLE_I3 1
#define ENABLE_CURL 1

#define WITH_XRANDR 1
#define WITH_XRENDER 0
#define WITH_XDAMAGE 0
#define WITH_XSYNC 0
#define WITH_XCOMPOSITE 0
#define WITH_XKB 1
#define XPP_EXTENSION_LIST xpp::randr::extension, xpp::xkb::extension

/* #undef DEBUG_LOGGER */
/* #undef VERBOSE_TRACELOG */
/* #undef DEBUG_HINTS */

static const size_t EVENT_SIZE{64UL};

static const int SIGN_PRIORITY_CONTROLLER{1};
static const int SIGN_PRIORITY_SCREEN{2};
static const int SIGN_PRIORITY_BAR{3};
static const int SIGN_PRIORITY_RENDERER{4};
static const int SIGN_PRIORITY_TRAY{5};

static const int SINK_PRIORITY_BAR{1};
static const int SINK_PRIORITY_SCREEN{2};
static const int SINK_PRIORITY_TRAY{3};
static const int SINK_PRIORITY_MODULE{4};

#ifdef DEBUG_HINTS
static const int DEBUG_HINTS_OFFSET_X{0};
static const int DEBUG_HINTS_OFFSET_Y{0};
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

const auto version_details = [](const std::vector<std::string>& args) {
  for (auto&& arg : args) {
    if (arg.compare(0, 3, "-vv") == 0)
      return true;
  }
  return false;
};

// clang-format off
const auto print_build_info = [](bool extended = false) {
  printf("%s %s\n\n", APP_NAME, APP_VERSION);
  printf("Features: %calsa %ccurl %ci3 %cmpd %cnetwork\n",
    (ENABLE_ALSA    ? '+' : '-'),
    (ENABLE_CURL    ? '+' : '-'),
    (ENABLE_I3      ? '+' : '-'),
    (ENABLE_MPD     ? '+' : '-'),
    (ENABLE_NETWORK ? '+' : '-'));
  if (extended) {
    printf("\n");
    printf("Build type: RelWithDebInfo\n");
    printf("Compiler: /usr/bin/c++\n");
    printf("Compiler flags:  -Wall -Wextra -Werror -O2 -pedantic -pedantic-errors\n");
    printf("Linker flags: \n");
    printf("\n");
    printf("ALSA_SOUNDCARD              %s\n", ALSA_SOUNDCARD);
    printf("BSPWM_SOCKET_PATH           %s\n", BSPWM_SOCKET_PATH);
    printf("BSPWM_STATUS_PREFIX         %s\n", BSPWM_STATUS_PREFIX);
    printf("BUILDER_SPACE_TOKEN         %s\n", BUILDER_SPACE_TOKEN);
    printf("CONNECTION_TEST_IP          %s\n", CONNECTION_TEST_IP);
    printf("PATH_ADAPTER                %s\n", PATH_ADAPTER);
    printf("PATH_BACKLIGHT_MAX          %s\n", PATH_BACKLIGHT_MAX);
    printf("PATH_BACKLIGHT_VAL          %s\n", PATH_BACKLIGHT_VAL);
    printf("PATH_BATTERY                %s\n", PATH_BATTERY);
    printf("PATH_CPU_INFO               %s\n", PATH_CPU_INFO);
    printf("PATH_MEMORY_INFO            %s\n", PATH_MEMORY_INFO);
    printf("PATH_TEMPERATURE_INFO       %s\n", PATH_TEMPERATURE_INFO);
  }
};
// clang-format on

// vim:ft=cpp
