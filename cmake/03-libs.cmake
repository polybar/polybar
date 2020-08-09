#
# Check libraries
#

find_package(Threads REQUIRED)
list(APPEND libs ${CMAKE_THREAD_LIBS_INIT})

querylib(TRUE "pkg-config" cairo-fc libs dirs)

querylib(ENABLE_ALSA "pkg-config" alsa libs dirs)
querylib(ENABLE_CURL "pkg-config" libcurl libs dirs)
querylib(ENABLE_MPD "pkg-config" libmpdclient libs dirs)
if(WITH_LIBNL)
  querylib(ENABLE_NETWORK "pkg-config" libnl-genl-3.0 libs dirs)
else()
  querylib(ENABLE_NETWORK "cmake" Libiw libs dirs)
endif()
querylib(ENABLE_PULSEAUDIO "pkg-config" libpulse libs dirs)

querylib(WITH_XCOMPOSITE "pkg-config" xcb-composite libs dirs)
querylib(WITH_XKB "pkg-config" xcb-xkb libs dirs)
querylib(WITH_XRANDR "pkg-config" xcb-randr libs dirs)
querylib(WITH_XRANDR_MONITORS "pkg-config" "xcb-randr>=1.12" libs dirs)
querylib(WITH_XRM "pkg-config" xcb-xrm libs dirs)
querylib(WITH_XCURSOR "pkg-config" xcb-cursor libs dirs)

# FreeBSD Support
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  querylib(TRUE "pkg-config" libinotify libs dirs)
endif()
