#
# Configure src
#

# Source tree {{{

file(GLOB_RECURSE files RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.c[p]*)
list(REMOVE_ITEM files main.cpp ipc.cpp)

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/settings.cpp.cmake
  ${CMAKE_BINARY_DIR}/generated-sources/settings.cpp
  ESCAPE_QUOTES)

list(APPEND files ${CMAKE_BINARY_DIR}/generated-sources/settings.cpp)

if(NOT ENABLE_ALSA)
  list(REMOVE_ITEM files modules/alsa.cpp)
  list(REMOVE_ITEM files adapters/alsa/control.cpp)
  list(REMOVE_ITEM files adapters/alsa/mixer.cpp)
endif()
if(NOT ENABLE_CURL)
  list(REMOVE_ITEM files modules/github.cpp)
  list(REMOVE_ITEM files utils/http.cpp)
endif()
if(NOT ENABLE_MPD)
  list(REMOVE_ITEM files modules/mpd.cpp)
  list(REMOVE_ITEM files adapters/mpd.cpp)
endif()
if(NOT ENABLE_NETWORK)
  list(REMOVE_ITEM files modules/network.cpp)
  list(REMOVE_ITEM files adapters/net.cpp)
  list(REMOVE_ITEM files adapters/net_iw.cpp)
  list(REMOVE_ITEM files adapters/net_nl.cpp)
endif()
if(WITH_LIBNL)
  list(REMOVE_ITEM files adapters/net_iw.cpp)
else()
  list(REMOVE_ITEM files adapters/net_nl.cpp)
endif()
if(NOT ENABLE_I3)
  list(REMOVE_ITEM files modules/i3.cpp)
  list(REMOVE_ITEM files utils/i3.cpp)
endif()
if(NOT ENABLE_PULSEAUDIO)
  list(REMOVE_ITEM files modules/pulseaudio.cpp)
  list(REMOVE_ITEM files adapters/pulseaudio.cpp)
endif()
if(NOT WITH_XRANDR)
  list(REMOVE_ITEM files x11/extensions/randr.cpp)
endif()
if(NOT WITH_XCOMPOSITE)
  list(REMOVE_ITEM files x11/extensions/composite.cpp)
endif()
if(NOT WITH_XKB)
  list(REMOVE_ITEM files x11/extensions/xkb.cpp)
  list(REMOVE_ITEM files modules/xkeyboard.cpp)
endif()
if(NOT WITH_XRM)
  list(REMOVE_ITEM files x11/xresources.cpp)
endif()
if(NOT WITH_XCURSOR)
  list(REMOVE_ITEM files x11/cursor.cpp)
endif()
# }}}

# Target: polybar {{{

add_library(poly STATIC ${files})
target_include_directories(poly PUBLIC ${dirs})
target_link_libraries(poly ${libs} Threads::Threads)
target_compile_options(poly PUBLIC $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)

add_executable(polybar main.cpp)
target_link_libraries(polybar poly)

install(TARGETS polybar
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT runtime)

# }}}
# Target: polybar-msg {{{

if(BUILD_IPC_MSG)
  add_executable(polybar-msg
    ipc.cpp
    utils/env.cpp
    utils/file.cpp
    utils/string.cpp)
  target_include_directories(polybar-msg PRIVATE ${dirs})
  target_compile_options(polybar-msg PUBLIC $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)

  install(TARGETS polybar-msg
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT runtime)

endif()

# }}}

# Export source file list so that it can be used for test compilation
set(files ${files} PARENT_SCOPE)
