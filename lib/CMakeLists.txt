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
add_library(xpp-interface INTERFACE)
target_link_libraries(xpp-interface INTERFACE ${XPP_LIBRARIES})
target_include_directories(xpp-interface INTERFACE ${XPP_INCLUDE_DIRS})
list(APPEND libs xpp-interface)

# }}}
# Library: i3ipcpp {{{

if(ENABLE_I3)
  add_subdirectory(i3ipcpp)
  list(APPEND libs i3ipc++)
endif()

# }}}

set(libs ${libs} PARENT_SCOPE)
