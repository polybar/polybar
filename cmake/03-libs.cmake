#
# Check libraries
#

find_package(Threads REQUIRED)
list(APPEND libs ${CMAKE_THREAD_LIBS_INIT})
list(APPEND libs ${CMAKE_DL_LIBS})

querylib(TRUE "pkg-config" cairo-fc libs dirs)
querylib(ENABLE_ALSA "pkg-config" alsa alsa-libs alsa-dirs)
querylib(ENABLE_CURL "pkg-config" libcurl curl-libs curl-dirs)
querylib(ENABLE_MPD "pkg-config" libmpdclient mpd-libs mpd-dirs)
if(WITH_LIBNL)
  querylib(ENABLE_NETWORK "pkg-config" libnl-genl-3.0 net-libs net-dirs)
else()
  querylib(ENABLE_NETWORK "cmake" Libiw net-libs net-dirs)
endif()
querylib(ENABLE_PULSEAUDIO "pkg-config" libpulse pulse-libs pulse-dirs)

querylib(WITH_XCOMPOSITE "pkg-config" xcb-composite libs dirs)
querylib(WITH_XDAMAGE "pkg-config" xcb-damage libs dirs)
querylib(WITH_XKB "pkg-config" xcb-xkb xkb-libs xkb-dirs)
querylib(WITH_XRANDR "pkg-config" xcb-randr libs dirs)
querylib(WITH_XRANDR_MONITORS "pkg-config" "xcb-randr>=1.12" libs dirs)
querylib(WITH_XRENDER "pkg-config" xcb-render libs dirs)
querylib(WITH_XRM "pkg-config" xcb-xrm libs dirs)
querylib(WITH_XSYNC "pkg-config" xcb-sync libs dirs)
querylib(WITH_XCURSOR "pkg-config" xcb-cursor libs dirs)

# FreeBSD Support
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  querylib(TRUE "pkg-config" libinotify libs dirs)
endif()
