#
# Configure libs
#

# Library: concurrentqueue {{{

add_library(concurrentqueue INTERFACE)
target_include_directories(concurrentqueue INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/concurrentqueue/include>)
list(APPEND libs concurrentqueue)

# }}}
# Library: xpp {{{

set(XCB_PROTOS xproto)

if(WITH_XRANDR)
  list(APPEND XCB_PROTOS randr)
endif()
if(WITH_XCOMPOSITE)
  list(APPEND XCB_PROTOS composite)
endif()
if(WITH_XKB)
  list(APPEND XCB_PROTOS xkb)
endif()

add_subdirectory(xpp)
list(APPEND libs xpp)

# }}}
# Library: i3ipcpp {{{

if(ENABLE_I3)
  add_subdirectory(i3ipcpp)
  list(APPEND libs ${I3IPCPP_LIBRARIES})
endif()

# }}}

set(libs ${libs} PARENT_SCOPE)
