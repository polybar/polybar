# Sets up options and dependencies for libpoly


# Automatically enable all optional dependencies that are available on the machine
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

# Load all packages for enabled components

find_package(Threads REQUIRED)
find_package(CairoFC REQUIRED)

find_package(LibUV 1.3.0 REQUIRED)

if (ENABLE_ALSA)
  find_package(ALSA REQUIRED)
endif()

if (ENABLE_CURL)
  find_package(CURL REQUIRED)
endif()

if (ENABLE_MPD)
  find_package(LibMPDClient REQUIRED)
  set(MPD_VERSION ${LibMPDClient_VERSION})
endif()

if (ENABLE_NETWORK)
  if(WITH_LIBNL)
    find_package(LibNlGenl3 REQUIRED)
    set(NETWORK_LIBRARY_VERSION ${LibNlGenl3_VERSION})
  else()
    find_package(Libiw REQUIRED)
  endif()
endif()

if (ENABLE_PULSEAUDIO)
  find_package(LibPulse REQUIRED)
  set(PULSEAUDIO_VERSION ${LibPulse_VERSION})
endif()

# xcomposite is required
list(APPEND XORG_EXTENSIONS COMPOSITE)
if (WITH_XKB)
  list(APPEND XORG_EXTENSIONS XKB)
endif()
if (WITH_XCURSOR)
  list(APPEND XORG_EXTENSIONS CURSOR)
endif()
if (WITH_XRM)
  list(APPEND XORG_EXTENSIONS XRM)
endif()

# Set min xrandr version required
if (WITH_XRANDR_MONITORS)
  set(XRANDR_VERSION "1.12")
else ()
  set(XRANDR_VERSION "")
endif()

# Randr is required. Searches for randr only because we may do a version check
find_package(Xcb ${XRANDR_VERSION} REQUIRED COMPONENTS RANDR)
find_package(Xcb REQUIRED COMPONENTS ${XORG_EXTENSIONS})

# FreeBSD Support
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  find_package(LibInotify REQUIRED)
endif()
