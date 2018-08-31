list(APPEND x11-sources
  atoms.cpp
  connection.cpp
  ewmh.cpp
  icccm.cpp
  registry.cpp
  tray_client.cpp
  tray_manager.cpp
  window.cpp
  winspec.cpp
  xembed.cpp)

if(WITH_XRANDR)
  list(APPEND x11-sources extensions/randr.cpp)
endif()
if(WITH_XRENDER)
  list(APPEND x11-sources extensions/render.cpp)
endif()
if(WITH_XDAMAGE)
  list(APPEND x11-sources extensions/damage.cpp)
endif()
if(WITH_XSYNC)
  list(APPEND x11-sources extensions/sync.cpp)
endif()
if(WITH_XCOMPOSITE)
  list(APPEND x11-sources extensions/composite.cpp)
endif()
if(WITH_XKB)
  list(APPEND x11-sources extensions/xkb.cpp)
endif()
if(WITH_XRM)
  list(APPEND x11-sources xresources.cpp)
endif()
if(WITH_XCURSOR)
  list(APPEND x11-sources cursor.cpp)
endif()

make_library(polybar-x11
  STATIC
  INTERNAL
  SOURCES
    ${x11-sources}
  INCLUDE_DIRS
    ${dirs}
  RAW_DEPENDS
    ${libs})
