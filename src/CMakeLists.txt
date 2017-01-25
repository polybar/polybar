#
# Create executable
#

file(GLOB_RECURSE SOURCES RELATIVE ${PROJECT_SOURCE_DIR}/src *.c[p]*)
list(REMOVE_ITEM SOURCES ipc.cpp)

# Locate dependencies {{{

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(PkgConfig)
find_package(Threads REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo-fc)

set(APP_LIBRARIES ${APP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
set(APP_LIBRARIES ${APP_LIBRARIES} ${CAIRO_LIBRARIES})

set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include)
set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/lib/concurrentqueue/include)
set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS})

# xpp library
set(XCB_PROTOS xproto)
if(WITH_XRANDR)
  if(ENABLE_XRANDR_MONITORS)
    pkg_check_modules(XRANDR REQUIRED xrandr>=1.5.0)
  endif()
  set(XCB_PROTOS "${XCB_PROTOS}" randr)
  set(XPP_EXTENSION_LIST ${XPP_EXTENSION_LIST} xpp::randr::extension)
else()
  list(REMOVE_ITEM SOURCES x11/extensions/randr.cpp)
endif()
if(WITH_XRENDER)
  set(XCB_PROTOS "${XCB_PROTOS}" render)
  set(XPP_EXTENSION_LIST ${XPP_EXTENSION_LIST} xpp::render::extension)
else()
  list(REMOVE_ITEM SOURCES x11/extensions/render.cpp)
endif()
if(WITH_XDAMAGE)
  set(XCB_PROTOS "${XCB_PROTOS}" damage)
  set(XPP_EXTENSION_LIST ${XPP_EXTENSION_LIST} xpp::damage::extension)
else()
  list(REMOVE_ITEM SOURCES x11/extensions/damage.cpp)
endif()
if(WITH_XSYNC)
  set(XCB_PROTOS "${XCB_PROTOS}" sync)
  set(XPP_EXTENSION_LIST ${XPP_EXTENSION_LIST} xpp::sync::extension)
else()
  list(REMOVE_ITEM SOURCES x11/extensions/sync.cpp)
endif()
if(WITH_XCOMPOSITE)
  set(XCB_PROTOS "${XCB_PROTOS}" composite)
  set(XPP_EXTENSION_LIST ${XPP_EXTENSION_LIST} xpp::composite::extension)
else()
  list(REMOVE_ITEM SOURCES x11/extensions/composite.cpp)
endif()
if(WITH_XKB)
  set(XCB_PROTOS "${XCB_PROTOS}" xkb)
  set(XPP_EXTENSION_LIST ${XPP_EXTENSION_LIST} xpp::xkb::extension)
else()
  list(REMOVE_ITEM SOURCES x11/extensions/xkb.cpp modules/xkeyboard.cpp)
endif()
if(WITH_XRM)
  pkg_check_modules(XCB_UTIL_XRM REQUIRED xcb-xrm)
  set(APP_LIBRARIES ${APP_LIBRARIES} ${XCB_UTIL_XRM_LIBRARIES})
  set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${XCB_UTIL_XRM_INCLUDE_DIRS})
endif()

string(REPLACE ";" ", " XPP_EXTENSION_LIST "${XPP_EXTENSION_LIST}")

add_subdirectory(${PROJECT_SOURCE_DIR}/lib/xpp ${PROJECT_BINARY_DIR}/lib/xpp)

set(APP_LIBRARIES ${APP_LIBRARIES} ${XPP_LIBRARIES})
set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${XPP_INCLUDE_DIRS})

# }}}
# Optional dependency: alsalib {{{

if(ENABLE_ALSA)
  find_package(ALSA REQUIRED)
  set(APP_LIBRARIES ${APP_LIBRARIES} ${ALSA_LIBRARY})
  set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${ALSA_INCLUDE_DIR})
else()
  list(REMOVE_ITEM SOURCES adapters/alsa/mixer.cpp adapters/alsa/control.cpp modules/volume.cpp)
endif()

# }}}
# Optional dependency: libmpdclient {{{

if(ENABLE_MPD)
  find_package(LibMPDClient REQUIRED)
  set(APP_LIBRARIES ${APP_LIBRARIES} ${LIBMPDCLIENT_LIBRARY})
  set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${LIBMPDCLIENT_INCLUDE_DIR})
else()
  list(REMOVE_ITEM SOURCES adapters/mpd.cpp modules/mpd.cpp)
endif()

# }}}
# Optional dependency: libiw {{{

if(ENABLE_NETWORK)
  find_package(Libiw REQUIRED)
  set(APP_LIBRARIES ${APP_LIBRARIES} ${LIBIW_LIBRARY})
  set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${LIBIW_INCLUDE_DIR})
else()
  list(REMOVE_ITEM SOURCES adapters/net.cpp modules/network.cpp)
endif()

# }}}
# Optional dependency: i3ipcpp {{{

if(ENABLE_I3)
  add_subdirectory(${PROJECT_SOURCE_DIR}/lib/i3ipcpp ${PROJECT_BINARY_DIR}/lib/i3ipcpp)
  set(APP_LIBRARIES ${APP_LIBRARIES} ${I3IPCPP_LIBRARIES})
  set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${I3IPCPP_INCLUDE_DIRS})
else()
  list(REMOVE_ITEM SOURCES modules/i3.cpp utils/i3.cpp)
endif()

# }}}
# Optional dependency: libcurl {{{

if(ENABLE_CURL)
  find_package(CURL REQUIRED)
  set(APP_LIBRARIES ${APP_LIBRARIES} ${CURL_LIBRARY})
  set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${CURL_INCLUDE_DIR})
else()
  list(REMOVE_ITEM SOURCES utils/http.cpp modules/github.cpp)
endif()

# }}}

# Create executable target {{{

make_executable(${PROJECT_NAME} SOURCES
    ${SOURCES}
  INCLUDE_DIRS
    ${APP_INCLUDE_DIRS}
  RAW_DEPENDS
    ${APP_LIBRARIES})

target_link_libraries(${PROJECT_NAME} Threads::Threads)

target_compile_options(${PROJECT_NAME} PUBLIC
  $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)

# }}}
# Create executable target: ipc messager {{{

if(BUILD_IPC_MSG)
  make_executable(${PROJECT_NAME}-msg SOURCES ipc.cpp utils/env.cpp utils/file.cpp)
  target_compile_options(${PROJECT_NAME}-msg PUBLIC
    $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)
endif()

# }}}

set(APP_BINARY ${PROJECT_SOURCE_DIR}/bin/${PROJECT_NAME} PARENT_SCOPE)
set(APP_LIBRARIES ${APP_LIBRARIES} PARENT_SCOPE)
set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} PARENT_SCOPE)

execute_process(COMMAND git describe --tags --dirty=-git
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE APP_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

configure_file(
  ${PROJECT_SOURCE_DIR}/include/settings.hpp.cmake
  ${CMAKE_SOURCE_DIR}/include/settings.hpp
  ESCAPE_QUOTES @ONLY)
