#include "settings.hpp"

const char* const APP_NAME{"@PROJECT_NAME@"};
const char* const APP_VERSION{"@APP_VERSION@"};

const int SINK_PRIORITY_BAR{1};
const int SINK_PRIORITY_SCREEN{2};
const int SINK_PRIORITY_TRAY{3};
const int SINK_PRIORITY_MODULE{4};

const char* const ALSA_SOUNDCARD{"@SETTING_ALSA_SOUNDCARD@"};
const char* const BSPWM_SOCKET_PATH{"@SETTING_BSPWM_SOCKET_PATH@"};
const char* const BSPWM_STATUS_PREFIX{"@SETTING_BSPWM_STATUS_PREFIX@"};
const char* const CONNECTION_TEST_IP{"@SETTING_CONNECTION_TEST_IP@"};
const char* const PATH_ADAPTER{"@SETTING_PATH_ADAPTER@"};
const char* const PATH_BACKLIGHT{"@SETTING_PATH_BACKLIGHT@"};
const char* const PATH_BATTERY{"@SETTING_PATH_BATTERY@"};
const char* const PATH_CPU_INFO{"@SETTING_PATH_CPU_INFO@"};
const char* const PATH_MEMORY_INFO{"@SETTING_PATH_MEMORY_INFO@"};
const char* const PATH_MESSAGING_FIFO{"@SETTING_PATH_MESSAGING_FIFO@"};
const char* const PATH_TEMPERATURE_INFO{"@SETTING_PATH_TEMPERATURE_INFO@"};
const char* const WIRELESS_LIB{"@WIRELESS_LIB@"};

bool version_details(const std::vector<std::string>& args) {
  for (auto&& arg : args) {
    if (arg.compare(0, 3, "-vv") == 0)
      return true;
  }
  return false;
}

// clang-format off
void print_build_info(bool extended) {
  printf("%s %s\n\n", APP_NAME, APP_VERSION);
  printf("Features: %calsa %ccurl %ci3 %cmpd %cnetwork(%s) %cpulseaudio %cxkeyboard\n",
    (ENABLE_ALSA       ? '+' : '-'),
    (ENABLE_CURL       ? '+' : '-'),
    (ENABLE_I3         ? '+' : '-'),
    (ENABLE_MPD        ? '+' : '-'),
    (ENABLE_NETWORK    ? '+' : '-'),
    WIRELESS_LIB,
    (ENABLE_PULSEAUDIO ? '+' : '-'),
    (ENABLE_XKEYBOARD  ? '+' : '-'));
  if (extended) {
    printf("\n");
    printf("X extensions: %crandr (%cmonitors) %ccomposite %cxkb %cxrm %cxcursor\n",
      (WITH_XRANDR            ? '+' : '-'),
      (WITH_XRANDR_MONITORS   ? '+' : '-'),
      (WITH_XCOMPOSITE        ? '+' : '-'),
      (WITH_XKB               ? '+' : '-'),
      (WITH_XRM               ? '+' : '-'),
      (WITH_XCURSOR           ? '+' : '-'));
    printf("\n");
    printf("Build type: @CMAKE_BUILD_TYPE@\n");
    printf("Compiler: @CMAKE_CXX_COMPILER@\n");
    printf("Compiler flags: @CMAKE_CXX_FLAGS@ ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}\n");
    printf("Linker flags: @CMAKE_EXE_LINKER_FLAGS@ ${CMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}\n");
  }
}
// clang-format on

// vim:ft=cpp
