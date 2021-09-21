#
# Configure libs
#

# Library: xpp {{{

set(XCB_PROTOS xproto)

list(APPEND XCB_PROTOS randr)
list(APPEND XCB_PROTOS composite)
if(WITH_XKB)
  list(APPEND XCB_PROTOS xkb)
endif()

add_subdirectory(xpp)
if(NOT TARGET xpp)
  message(FATAL_ERROR "Target xpp not generated")
else()
  get_target_property(_xpp_includes xpp INCLUDE_DIRECTORIES)
  set_target_properties(xpp PROPERTIES INCLUDE_DIRECTORIES "")
  target_include_directories(xpp SYSTEM PUBLIC ${_xpp_includes})
endif()

# }}}
# Library: i3ipcpp {{{

if(ENABLE_I3)
  add_subdirectory(i3ipcpp)
  if(NOT TARGET i3ipc++)
    message(FATAL_ERROR "Target i3ipcpp not generated")
  else()
    get_target_property(_i3ipcpp_includes i3ipc++ INCLUDE_DIRECTORIES)
    set_target_properties(i3ipc++ PROPERTIES INCLUDE_DIRECTORIES "")
    target_include_directories(i3ipc++ SYSTEM PUBLIC ${_i3ipcpp_includes})
  endif()
endif()

# }}}
