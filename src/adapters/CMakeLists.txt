#
# Check libraries
#

querylib(ENABLE_ALSA "pkg-config" alsa alsa-libs alsa-dirs)
querylib(ENABLE_MPD "pkg-config" libmpdclient mpd-libs mpd-dirs)
if(WITH_LIBNL)
  querylib(ENABLE_NETWORK "pkg-config" libnl-genl-3.0 net-libs net-dirs)
else()
  querylib(ENABLE_NETWORK "cmake" Libiw net-libs net-dirs)
endif()
querylib(ENABLE_PULSEAUDIO "pkg-config" libpulse pulse-libs pulse-dirs)

#
# Add adapters as optional shared libraries
#

if(ENABLE_ALSA)
  make_library(polybar-alsa-adapter
    STATIC
    SOURCES
      alsa/control.cpp
      alsa/mixer.cpp
    INCLUDE_DIRS
      ${dirs}
      ${alsa-dirs}
    RAW_DEPENDS
      ${alsa-libs}
      Threads::Threads)
endif()

if(ENABLE_MPD)
  make_library(polybar-mpd-adapter
    STATIC
    SOURCES
      mpd.cpp
    INCLUDE_DIRS
      ${dirs}
      ${mpd-dirs}
    RAW_DEPENDS
      ${mpd-libs}
      Threads::Threads)
endif()

if(ENABLE_NETWORK)
  list(APPEND net_src net.cpp)
  if(WITH_LIBNL)
    list(APPEND net_src net_nl.cpp)
  else()
    list(APPEND net_src net_iw.cpp)
  endif()
  make_library(polybar-net-adapter
    STATIC
    SOURCES
      ${net_src}
    INCLUDE_DIRS
      ${dirs}
      ${net-dirs}
    RAW_DEPENDS
      ${net-libs}
      Threads::Threads)
endif()

if(ENABLE_PULSEAUDIO)
  make_library(polybar-pulse-adapter
    STATIC
    SOURCES
      pulseaudio.cpp
    INCLUDE_DIRS
      ${dirs}
      ${pulse-dirs}
    RAW_DEPENDS
      ${pulse-libs}
      Threads::Threads)
endif()
