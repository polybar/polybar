#
# Configure src
#

# Source tree {{{

file(GLOB_RECURSE files RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.c[p]*)
list(REMOVE_ITEM files ipc.cpp)

if(NOT ENABLE_ALSA)
  list(REMOVE_ITEM files modules/volume.cpp)
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
if(NOT ENABLE_MPRIS)
  list(REMOVE_ITEM files modules/mpris.cpp)
  list(REMOVE_ITEM files adapters/mpris.cpp)
endif()
if(NOT ENABLE_NETWORK)
  list(REMOVE_ITEM files modules/network.cpp)
  list(REMOVE_ITEM files adapters/net.cpp)
endif()
if(NOT ENABLE_I3)
  list(REMOVE_ITEM files modules/i3.cpp)
  list(REMOVE_ITEM files utils/i3.cpp)
endif()
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
endif()
if(NOT WITH_XRM)
  list(REMOVE_ITEM files x11/xresources.cpp)
endif()

# }}}

# Target: polybar {{{

make_executable(polybar
  SOURCES
    ${files}
  INCLUDE_DIRS
    ${dirs}
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
      utils/file.cpp)
  target_compile_options(polybar-msg PUBLIC $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)
endif()

# }}}

set(libs ${libs} PARENT_SCOPE)
set(dirs ${dirs} PARENT_SCOPE)
