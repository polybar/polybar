#
# Build options
#

# Default value for: ENABLE_ALSA {{{

find_package(ALSA QUIET)
if(NOT DEFINED ENABLE_ALSA AND NOT ALSA_FOUND)
  set(ENABLE_ALSA OFF CACHE STRING "Module support for alsa-lib")
endif()

# }}}
# Default value for: ENABLE_NETWORK {{{

find_package(Libiw QUIET)
if(NOT DEFINED ENABLE_NETWORK AND NOT LIBIW_FOUND)
  set(ENABLE_NETWORK OFF CACHE STRING "Module support for wireless_tools")
endif()

# }}}
# Default value for: ENABLE_MPD {{{

find_package(LibMPDClient QUIET)
if(NOT DEFINED ENABLE_MPD AND NOT LIBMPDCLIENT_FOUND)
  set(ENABLE_MPD OFF CACHE STRING "Module support for libmpdclient")
endif()

# }}}
# Default value for: ENABLE_I3 {{{

find_program(I3_BINARY i3)
if(NOT DEFINED ENABLE_I3 AND NOT I3_BINARY)
  set(ENABLE_I3 OFF CACHE STRING "Module support for i3wm")
endif()

# }}}
# Default value for: ENABLE_CURL {{{

find_package(CURL QUIET)
if(NOT DEFINED ENABLE_CURL AND NOT CURL_FOUND)
  set(ENABLE_CURL OFF CACHE STRING "Module support for libcurl")
endif()

# }}}

# Define build options {{{

option(CXXLIB_CLANG       "Link against libc++"        OFF)
option(CXXLIB_GCC         "Link against stdlibc++"     OFF)

option(BUILD_IPC_MSG      "Build ipc messager"         ON)
option(BUILD_TESTS        "Build testsuite"            OFF)

if(DEBUG)
  option(DEBUG_LOGGER         "Debug logging"            OFF)
  option(DEBUG_LOGGER_VERBOSE "Debug logging (verbose)"  OFF)
  option(DEBUG_HINTS          "Debug clickable areas"    OFF)
  option(DEBUG_WHITESPACE     "Debug whitespace"         OFF)
  option(DEBUG_FONTCONFIG     "Debug fontconfig"         OFF)
endif()

option(ENABLE_CCACHE      "Enable ccache support"      OFF)
option(ENABLE_ALSA        "Enable alsa support"        ON)
option(ENABLE_CURL        "Enable curl support"        ON)
option(ENABLE_I3          "Enable i3 support"          ON)
option(ENABLE_MPD         "Enable mpd support"         ON)
option(ENABLE_NETWORK     "Enable network support"     ON)

option(WITH_XRANDR        "xcb-randr support"          ON)
option(WITH_XRENDER       "xcb-render support"         OFF)
option(WITH_XDAMAGE       "xcb-damage support"         OFF)
option(WITH_XSYNC         "xcb-sync support"           OFF)
option(WITH_XCOMPOSITE    "xcb-composite support"      OFF)
option(WITH_XKB           "xcb-xkb support"            ON)
option(WITH_XRM           "xcb-xrm support"            ON)

if(NOT DEFINED WITH_XRM)
  pkg_check_modules(XRM QUIET xcb-xrm)
  set(WITH_XRM ${XRM_FOUND} CACHE BOOL "Enable xcb-xrm support")
endif()

if(NOT DEFINED ENABLE_XRANDR_MONITORS)
  pkg_check_modules(XRANDR QUIET xrandr>=1.5.0)
  if(NOT XRANDR_FOUND)
    set(XRANDR_FOUND OFF)
  endif()
  set(ENABLE_XRANDR_MONITORS ${XRANDR_FOUND} CACHE BOOL "Enable XRandR monitor feature (requires version 1.5+)")
endif()

# }}}
# Set cache vars {{{

set(SETTING_ALSA_SOUNDCARD "default"
  CACHE STRING "Name of the ALSA soundcard driver")
set(SETTING_BSPWM_SOCKET_PATH "/tmp/bspwm_0_0-socket"
  CACHE STRING "Path to bspwm socket")
set(SETTING_BSPWM_STATUS_PREFIX "W"
  CACHE STRING "Prefix prepended to the bspwm status line")
set(SETTING_CONNECTION_TEST_IP "8.8.8.8"
  CACHE STRING "Address to ping when testing network connection")
set(SETTING_PATH_ADAPTER "/sys/class/power_supply/%adapter%"
  CACHE STRING "Path to adapter")
set(SETTING_PATH_BACKLIGHT_MAX "/sys/class/backlight/%card%/max_brightness"
  CACHE STRING "Path to file containing the maximum backlight value")
set(SETTING_PATH_BACKLIGHT_VAL "/sys/class/backlight/%card%/brightness"
  CACHE STRING "Path to file containing the current backlight value")
set(SETTING_PATH_BATTERY "/sys/class/power_supply/%battery%"
  CACHE STRING "Path to battery")
set(SETTING_PATH_CPU_INFO "/proc/stat"
  CACHE STRING "Path to file containing cpu info")
set(SETTING_PATH_MEMORY_INFO "/proc/meminfo"
  CACHE STRING "Path to file containing memory info")
set(SETTING_PATH_MESSAGING_FIFO "/tmp/polybar_mqueue.%pid%"
  CACHE STRING "Path to file containing the current temperature")
set(SETTING_PATH_TEMPERATURE_INFO "/sys/class/thermal/thermal_zone%zone%/temp"
  CACHE STRING "Path to file containing the current temperature")

set(DEBUG_HINTS_OFFSET_X 0 CACHE INTEGER "Debug hint offset x")
set(DEBUG_HINTS_OFFSET_Y 0 CACHE INTEGER "Debug hint offset y")

# }}}
