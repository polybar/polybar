querylib(ENABLE_ALSA "pkg-config" alsa alsa-libs alsa-dirs)
querylib(ENABLE_CURL "pkg-config" libcurl curl-libs curl-dirs)
querylib(ENABLE_MPD "pkg-config" libmpdclient mpd-libs mpd-dirs)
if(WITH_LIBNL)
  querylib(ENABLE_NETWORK "pkg-config" libnl-genl-3.0 net-libs net-dirs)
  list(APPEND net-src ../adapters/net_nl.cpp)
else()
  querylib(ENABLE_NETWORK "cmake" Libiw net-libs net-dirs)
  list(APPEND net-src ../adapters/net_iw.cpp)
endif()
querylib(ENABLE_PULSEAUDIO "pkg-config" libpulse pulse-libs pulse-dirs)

if(ENABLE_ALSA)
  make_library(polybar-alsa
    SHARED
    SOURCES
      ../adapters/alsa/control.cpp
      ../adapters/alsa/mixer.cpp
      alsa.cpp
    INCLUDE_DIRS
      ${dirs}
      ${alsa-dirs}
    RAW_DEPENDS
      ${alsa-libs})
  list(APPEND shared_modules polybar-alsa_shared)
endif()

if(ENABLE_CURL)
  make_library(polybar-github
    SHARED
    SOURCES
      ../utils/http.cpp
      github.cpp
    INCLUDE_DIRS
      ${dirs}
      ${curl-dirs}
    RAW_DEPENDS
      ${curl-libs})
  list(APPEND shared_modules polybar-github_shared)
endif()

if(ENABLE_MPD)
  make_library(polybar-mpd
    SHARED
    SOURCES
      ../adapters/mpd.cpp
      mpd.cpp
    INCLUDE_DIRS
      ${dirs}
      ${mpd-dirs}
    RAW_DEPENDS
      ${mpd-libs})
  list(APPEND shared_modules polybar-mpd_shared)
endif()

if(ENABLE_NETWORK)
  list(APPEND net-src ../adapters/net.cpp network.cpp)
  make_library(polybar-network
    SHARED
    SOURCES
      ${net-src}
    INCLUDE_DIRS
      ${dirs}
      ${net-dirs}
    RAW_DEPENDS
      ${net-libs})
  list(APPEND shared_modules polybar-network_shared)
endif()

if(ENABLE_I3)
  make_library(polybar-i3
    SHARED
    SOURCES
      ../utils/i3.cpp
      i3.cpp
    INCLUDE_DIRS
      ${dirs}
      ${I3IPCPP_INCLUDE_DIRS}
    RAW_DEPENDS
      ${I3IPCPP_LIBRARIES})
  list(APPEND shared_modules polybar-i3_shared)
endif()

if(ENABLE_PULSEAUDIO)
  make_library(polybar-pulseaudio
    SHARED
    SOURCES
      ../adapters/pulseaudio.cpp
      pulseaudio.cpp
    INCLUDE_DIRS
      ${dirs}
      ${pulse-dirs}
    RAW_DEPENDS
      ${pulse-libs})
    list(APPEND shared_modules polybar-pulseaudio_shared)
endif()

set(shared_modules ${shared_modules} PARENT_SCOPE)
