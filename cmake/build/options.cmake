#
# Build options
#

option(CXXLIB_CLANG       "Link against libc++"        OFF)
option(CXXLIB_GCC         "Link against stdlibc++"     OFF)

option(BUILD_TESTS        "Build testsuite"            OFF)
option(DEBUG_LOGGER       "Enable extra debug logging" OFF)
option(VERBOSE_TRACELOG   "Enable verbose trace logs"  OFF)

option(ENABLE_CCACHE      "Enable ccache support"      OFF)
option(ENABLE_ALSA        "Enable alsa support"        ON)
option(ENABLE_I3          "Enable i3 support"          ON)
option(ENABLE_MPD         "Enable mpd support"         ON)
option(ENABLE_NETWORK     "Enable network support"     ON)

option(ENABLE_RANDR_EXT   "Enable RandR X extension"   ON)
option(ENABLE_RENDER_EXT  "Enable Render X extension"  OFF)
option(ENABLE_DAMAGE_EXT  "Enable Damage X extension"  OFF)

set(SETTING_ALSA_SOUNDCARD "default"
  CACHE STRING "Name of the ALSA soundcard driver")
set(SETTING_CONNECTION_TEST_IP "8.8.8.8"
  CACHE STRING "Address to ping when testing network connection")
set(SETTING_PATH_BACKLIGHT_VAL "/sys/class/backlight/%card%/brightness"
  CACHE STRING "Path to file containing the current backlight value")
set(SETTING_PATH_BACKLIGHT_MAX "/sys/class/backlight/%card%/max_brightness"
  CACHE STRING "Path to file containing the maximum backlight value")
set(SETTING_PATH_BATTERY_CAPACITY "/sys/class/power_supply/%battery%/capacity"
  CACHE STRING "Path to file containing the current battery capacity")
set(SETTING_PATH_ADAPTER_STATUS "/sys/class/power_supply/%adapter%/online"
  CACHE STRING "Path to file containing the current adapter status")
set(SETTING_BSPWM_SOCKET_PATH "/tmp/bspwm_0_0-socket"
  CACHE STRING "Path to bspwm socket")
set(SETTING_BSPWM_STATUS_PREFIX "W"
  CACHE STRING "Prefix prepended to the bspwm status line")
set(SETTING_PATH_CPU_INFO "/proc/stat"
  CACHE STRING "Path to file containing cpu info")
set(SETTING_PATH_MEMORY_INFO "/proc/meminfo"
  CACHE STRING "Path to file containing memory info")

find_package(ALSA QUIET)
if(NOT ALSA_FOUND)
  set(ENABLE_ALSA OFF)
endif()
find_package(Libiw QUIET)
if(NOT LIBIW_FOUND)
  set(ENABLE_NETWORK OFF)
endif()
find_package(LibMPDClient QUIET)
if(NOT LIBMPDCLIENT_FOUND)
  set(ENABLE_MPD OFF)
endif()
find_program(I3_BINARY i3)
if(NOT I3_BINARY)
  set(ENABLE_I3 OFF)
endif()
