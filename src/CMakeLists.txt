#
# Configure src
#

add_subdirectory(adapters)

# Source tree {{{

file(GLOB_RECURSE files RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.c[p]*)
list(REMOVE_ITEM files ipc.cpp)

if(ENABLE_ALSA)
  list(APPEND target_deps polybar-alsa-adapter_static)
else()
  list(REMOVE_ITEM files modules/alsa.cpp)
endif()
list(REMOVE_ITEM files adapters/alsa/control.cpp)
list(REMOVE_ITEM files adapters/alsa/mixer.cpp)
if(NOT ENABLE_CURL)
  list(REMOVE_ITEM files modules/github.cpp)
  list(REMOVE_ITEM files utils/http.cpp)
endif()
if(ENABLE_MPD)
  list(APPEND target_deps polybar-mpd-adapter_static)
else()
  list(REMOVE_ITEM files modules/mpd.cpp)
endif()
list(REMOVE_ITEM files adapters/mpd.cpp)
if(ENABLE_NETWORK)
  list(APPEND target_deps polybar-net-adapter_static)
else()
  list(REMOVE_ITEM files modules/network.cpp)
endif()
list(REMOVE_ITEM files adapters/net.cpp)
list(REMOVE_ITEM files adapters/net_iw.cpp)
list(REMOVE_ITEM files adapters/net_nl.cpp)
if(NOT ENABLE_I3)
  list(REMOVE_ITEM files modules/i3.cpp)
  list(REMOVE_ITEM files utils/i3.cpp)
endif()
if(ENABLE_PULSEAUDIO)
  list(APPEND target_deps polybar-pulse-adapter_static)
else()
  list(REMOVE_ITEM files modules/pulseaudio.cpp)
endif()
list(REMOVE_ITEM files adapters/pulseaudio.cpp)
if(NOT WITH_XRANDR)
  list(REMOVE_ITEM files x11/extensions/randr.cpp)
endif()
if(NOT WITH_XRENDER)
  list(REMOVE_ITEM files x11/extensions/render.cpp)
endif()
if(NOT WITH_XDAMAGE)
  list(REMOVE_ITEM files x11/extensions/damage.cpp)
endif()
if(NOT WITH_XSYNC)
  list(REMOVE_ITEM files x11/extensions/sync.cpp)
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

make_executable(polybar
  SOURCES
    ${files}
  INCLUDE_DIRS
    ${dirs}
  TARGET_DEPENDS
    ${target_deps}
  RAW_DEPENDS
    ${libs}
    Threads::Threads)

target_compile_options(polybar PUBLIC $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)

# }}}
# Target: polybar-msg {{{

if(BUILD_IPC_MSG)
  make_executable(polybar-msg
    SOURCES
      ipc.cpp
      utils/env.cpp
      utils/file.cpp
      utils/string.cpp)
  target_compile_options(polybar-msg PUBLIC $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)
endif()

# }}}

# Export source file list so that it can be used for test compilation
set(files ${files} PARENT_SCOPE)
set(libs ${libs} PARENT_SCOPE)
set(dirs ${dirs} PARENT_SCOPE)
