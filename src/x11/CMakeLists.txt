add_sources(files
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
  add_sources(files extensions/randr.cpp)
endif()
if(WITH_XRENDER)
  add_sources(files extensions/render.cpp)
endif()
if(WITH_XDAMAGE)
  add_sources(files extensions/damage.cpp)
endif()
if(WITH_XSYNC)
  add_sources(files extensions/sync.cpp)
endif()
if(WITH_XCOMPOSITE)
  add_sources(files extensions/composite.cpp)
endif()
if(WITH_XKB)
  add_sources(files extensions/xkb.cpp)
endif()
if(WITH_XRM)
  add_sources(files xresources.cpp)
endif()
if(WITH_XCURSOR)
  add_sources(files cursor.cpp)
endif()
