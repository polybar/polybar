if(ENABLE_ALSA)
  make_library(polybar-modules-alsa
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
  list(APPEND shared_modules polybar-modules-alsa_shared)
endif()

if(ENABLE_CURL)
  make_library(polybar-modules-github
    SHARED
    SOURCES
      ../utils/http.cpp
      github.cpp
    INCLUDE_DIRS
      ${dirs}
      ${curl-dirs}
    RAW_DEPENDS
      ${curl-libs})
  list(APPEND shared_modules polybar-modules-github_shared)
endif()

if(ENABLE_MPD)
  make_library(polybar-modules-mpd
    SHARED
    SOURCES
      ../adapters/mpd.cpp
      mpd.cpp
    INCLUDE_DIRS
      ${dirs}
      ${mpd-dirs}
    RAW_DEPENDS
      ${mpd-libs})
  list(APPEND shared_modules polybar-modules-mpd_shared)
endif()

if(ENABLE_NETWORK)
  list(APPEND net-src ../adapters/net.cpp network.cpp)
  make_library(polybar-modules-network
    SHARED
    SOURCES
      ${net-src}
    INCLUDE_DIRS
      ${dirs}
      ${net-dirs}
    RAW_DEPENDS
      ${net-libs})
  list(APPEND shared_modules polybar-modules-network_shared)
endif()

if(ENABLE_I3)
  make_library(polybar-modules-i3
    SHARED
    SOURCES
      ../utils/i3.cpp
      i3.cpp
    INCLUDE_DIRS
      ${dirs}
      ${I3IPCPP_INCLUDE_DIRS}
    RAW_DEPENDS
      ${I3IPCPP_LIBRARIES})
  list(APPEND shared_modules polybar-modules-i3_shared)
endif()

if(ENABLE_PULSEAUDIO)
  make_library(polybar-modules-pulseaudio
    SHARED
    SOURCES
      ../adapters/pulseaudio.cpp
      pulseaudio.cpp
    INCLUDE_DIRS
      ${dirs}
      ${pulse-dirs}
    RAW_DEPENDS
      ${pulse-libs})
  list(APPEND shared_modules polybar-modules-pulseaudio_shared)
endif()

if(WITH_XKB)
  make_library(polybar-modules-xkeyboard
    SHARED
    SOURCES
      ../x11/extensions/xkb.cpp
      xkeyboard.cpp
    INCLUDE_DIRS
      ${dirs}
      ${xkb-dirs}
    RAW_DEPENDS
      ${xkb-libs})
  list(APPEND shared_modules polybar-modules-xkeyboard_shared)
endif()

make_library(polybar-modules-internal
  SHARED
  SOURCES
    backlight.cpp
    battery.cpp
    bspwm.cpp
    counter.cpp
    cpu.cpp
    date.cpp
    fs.cpp
    ipc.cpp
    memory.cpp
    menu.cpp
    meta/base.cpp
    meta/factory.cpp
    script.cpp
    systray.cpp
    temperature.cpp
    text.cpp
    xbacklight.cpp
    xwindow.cpp
    xworkspaces.cpp
  INCLUDE_DIRS
    ${dirs})
list(APPEND shared_modules polybar-modules-internal_shared)

set(shared_modules ${shared_modules} PARENT_SCOPE)
