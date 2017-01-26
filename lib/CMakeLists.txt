#
# Configure libs
#

# Library: concurrentqueue {{{

list(APPEND dirs ${CMAKE_CURRENT_LIST_DIR}/concurrentqueue/include)

# }}}
# Library: xpp {{{

set(XCB_PROTOS xproto)

if(WITH_XRANDR)
  list(APPEND XCB_PROTOS randr)
endif()
if(WITH_XRENDER)
  list(APPEND XCB_PROTOS render)
endif()
if(WITH_XDAMAGE)
  list(APPEND XCB_PROTOS damage)
endif()
if(WITH_XSYNC)
  list(APPEND XCB_PROTOS sync)
endif()
if(WITH_XCOMPOSITE)
  list(APPEND XCB_PROTOS composite)
endif()
if(WITH_XKB)
  list(APPEND XCB_PROTOS xkb)
endif()

add_subdirectory(xpp)
list(APPEND libs ${XPP_LIBRARIES})
list(APPEND dirs ${XPP_INCLUDE_DIRS})

# }}}
# Library: i3ipcpp {{{

if(ENABLE_I3)
  add_subdirectory(i3ipcpp)
  list(APPEND libs ${I3IPCPP_LIBRARIES})
  list(APPEND dirs ${I3IPCPP_INCLUDE_DIRS})
endif()

# }}}

set(libs ${libs} PARENT_SCOPE)
set(dirs ${dirs} PARENT_SCOPE)
