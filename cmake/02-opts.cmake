set(SPHINX_FLAGS "" CACHE STRING "Flags to pass to sphinx-build")
set(SPHINX_BUILD "sphinx-build" CACHE STRING "Name/Path of the sphinx-build executable to use.")

if (HAS_CXX_COMPILATION)
  option(ENABLE_CCACHE "Enable ccache support" ON)
  if(ENABLE_CCACHE)
    find_program(BIN_CCACHE ccache)
    mark_as_advanced(BIN_CCACHE)

    if(NOT BIN_CCACHE)
      message_colored(STATUS "Couldn't locate ccache, disabling ccache..." "33")
    else()
      # Enable only if the binary is found
      message_colored(STATUS "Using compiler cache ${BIN_CCACHE}" "32")
      set(CMAKE_CXX_COMPILER_LAUNCHER ${BIN_CCACHE} CACHE STRING "")
    endif()
  endif()

  option(CXXLIB_CLANG "Link against libc++" OFF)
  option(CXXLIB_GCC "Link against stdlibc++" OFF)
endif()

if (BUILD_LIBPOLY)
  checklib(ENABLE_ALSA "pkg-config" alsa)
  checklib(ENABLE_CURL "pkg-config" libcurl)
  checklib(ENABLE_I3 "binary" i3)
  checklib(ENABLE_MPD "pkg-config" libmpdclient)
  checklib(WITH_LIBNL "pkg-config" libnl-genl-3.0)
  if(WITH_LIBNL)
    checklib(ENABLE_NETWORK "pkg-config" libnl-genl-3.0)
    set(WIRELESS_LIB "libnl")
  else()
    checklib(ENABLE_NETWORK "cmake" Libiw)
    set(WIRELESS_LIB "wireless-tools")
  endif()
  checklib(ENABLE_PULSEAUDIO "pkg-config" libpulse)
  checklib(WITH_XKB "pkg-config" xcb-xkb)
  checklib(WITH_XRM "pkg-config" xcb-xrm)
  checklib(WITH_XRANDR_MONITORS "pkg-config" "xcb-randr>=1.12")
  checklib(WITH_XCURSOR "pkg-config" "xcb-cursor")

  option(ENABLE_ALSA "Enable alsa support" ON)
  option(ENABLE_CURL "Enable curl support" ON)
  option(ENABLE_I3 "Enable i3 support" ON)
  option(ENABLE_MPD "Enable mpd support" ON)
  option(WITH_LIBNL "Use netlink interface for wireless" ON)
  option(ENABLE_NETWORK "Enable network support" ON)
  option(ENABLE_XKEYBOARD "Enable xkeyboard support" ON)
  option(ENABLE_PULSEAUDIO "Enable PulseAudio support" ON)

  option(WITH_XRANDR_MONITORS "xcb-randr monitor support" ON)
  option(WITH_XKB "xcb-xkb support" ON)
  option(WITH_XRM "xcb-xrm support" ON)
  option(WITH_XCURSOR "xcb-cursor support" ON)

  option(DEBUG_LOGGER "Trace logging" ON)

  if(CMAKE_BUILD_TYPE_UPPER MATCHES DEBUG)
    option(DEBUG_LOGGER_VERBOSE "Trace logging (verbose)" OFF)
    option(DEBUG_HINTS "Debug clickable areas" OFF)
    option(DEBUG_WHITESPACE "Debug whitespace" OFF)
    option(DEBUG_FONTCONFIG "Debug fontconfig" OFF)
  endif()
endif()

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
set(SETTING_PATH_BACKLIGHT "/sys/class/backlight/%card%"
  CACHE STRING "Path to backlight sysfs folder")
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
