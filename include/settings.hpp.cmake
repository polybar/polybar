#pragma once

#include <cstdio>
#include <string>
#include <vector>

extern const char* const APP_NAME;
extern const char* const APP_VERSION;

#cmakedefine01 ENABLE_ALSA
#cmakedefine01 ENABLE_MPD
#cmakedefine01 ENABLE_NETWORK
#cmakedefine01 WITH_LIBNL
#cmakedefine01 ENABLE_I3
#cmakedefine01 ENABLE_CURL
#cmakedefine01 ENABLE_PULSEAUDIO

#cmakedefine01 WITH_XRANDR
#cmakedefine01 WITH_XCOMPOSITE
#cmakedefine01 WITH_XKB
#cmakedefine01 WITH_XRM
#cmakedefine01 WITH_XCURSOR

#if WITH_XRANDR
#cmakedefine01 WITH_XRANDR_MONITORS
#else
#define WITH_XRANDR_MONITORS 0
#endif

#if WITH_XKB
#cmakedefine01 ENABLE_XKEYBOARD
#else
#define ENABLE_XKEYBOARD 0
#endif

#cmakedefine XPP_EXTENSION_LIST @XPP_EXTENSION_LIST@

#cmakedefine DEBUG_LOGGER

#if DEBUG
#cmakedefine DEBUG_LOGGER_VERBOSE
#cmakedefine DEBUG_HINTS
#cmakedefine DEBUG_WHITESPACE
#cmakedefine DEBUG_SHADED
#cmakedefine DEBUG_FONTCONFIG
#endif

static const int SIGN_PRIORITY_CONTROLLER{1};
static const int SIGN_PRIORITY_SCREEN{2};
static const int SIGN_PRIORITY_BAR{3};
static const int SIGN_PRIORITY_RENDERER{4};
static const int SIGN_PRIORITY_TRAY{5};

extern const int SINK_PRIORITY_BAR;
extern const int SINK_PRIORITY_SCREEN;
extern const int SINK_PRIORITY_TRAY;
extern const int SINK_PRIORITY_MODULE;

extern const char* const ALSA_SOUNDCARD;
extern const char* const BSPWM_SOCKET_PATH;
extern const char* const BSPWM_STATUS_PREFIX;
extern const char* const CONNECTION_TEST_IP;
extern const char* const PATH_ADAPTER;
extern const char* const PATH_BACKLIGHT;
extern const char* const PATH_BATTERY;
extern const char* const PATH_CPU_INFO;
extern const char* const PATH_MEMORY_INFO;
extern const char* const PATH_MESSAGING_FIFO;
extern const char* const PATH_TEMPERATURE_INFO;
extern const char* const WIRELESS_LIB;

bool version_details(const std::vector<std::string>& args);

void print_build_info(bool extended = false);

// vim:ft=cpp
