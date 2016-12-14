#
# Create executable
#

# Generate source tree {{{

file(GLOB_RECURSE SOURCES RELATIVE ${PROJECT_SOURCE_DIR}/src *.c[p]*)

if(NOT ENABLE_ALSA)
  list(REMOVE_ITEM SOURCES adapters/alsa.cpp modules/volume.cpp)
endif()
if(NOT ENABLE_MPD)
  list(REMOVE_ITEM SOURCES adapters/mpd.cpp modules/mpd.cpp)
endif()
if(NOT ENABLE_NETWORK)
  list(REMOVE_ITEM SOURCES adapters/net.cpp modules/network.cpp)
endif()
if(NOT ENABLE_I3)
  list(REMOVE_ITEM SOURCES modules/i3.cpp utils/i3.cpp)
endif()

if(NOT WITH_XCOMPOSITE)
  list(REMOVE_ITEM SOURCES x11/composite.cpp)
endif()
if(NOT WITH_XDAMAGE)
  list(REMOVE_ITEM SOURCES x11/damage.cpp)
endif()
if(NOT WITH_XRANDR)
  list(REMOVE_ITEM SOURCES x11/randr.cpp)
endif()
if(NOT WITH_XRENDER)
  list(REMOVE_ITEM SOURCES x11/render.cpp)
endif()
if(NOT WITH_XSYNC)
  list(REMOVE_ITEM SOURCES x11/sync.cpp)
endif()
if(NOT WITH_XKB)
  list(REMOVE_ITEM SOURCES x11/xkb.cpp modules/xkeyboard.cpp)
endif()

# }}}
# Locate dependencies {{{

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(X11 REQUIRED COMPONENTS Xft Xutil)
find_package(X11_XCB REQUIRED)
find_package(PkgConfig)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)

set(APP_LIBRARIES ${APP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
set(APP_LIBRARIES ${APP_LIBRARIES} ${X11_Xft_LIB})
set(APP_LIBRARIES ${APP_LIBRARIES} ${FONTCONFIG_LIBRARIES})

set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${FONTCONFIG_INCLUDE_DIRS})
set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include)
set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/lib/concurrentqueue/include)
set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

# xpp library
set(XCB_PROTOS xproto)
if(WITH_XRANDR)
  set(XCB_PROTOS "${XCB_PROTOS}" randr)
  set(XPP_EXTENSION_LIST ${XPP_EXTENSION_LIST} xpp::randr::extension)
endif()
if(WITH_XRENDER)
  set(XCB_PROTOS "${XCB_PROTOS}" render)
  set(XPP_EXTENSION_LIST ${XPP_EXTENSION_LIST} xpp::render::extension)
endif()
if(WITH_XDAMAGE)
  set(XCB_PROTOS "${XCB_PROTOS}" damage)
  set(XPP_EXTENSION_LIST ${XPP_EXTENSION_LIST} xpp::damage::extension)
endif()
if(WITH_XSYNC)
  set(XCB_PROTOS "${XCB_PROTOS}" sync)
  set(XPP_EXTENSION_LIST ${XPP_EXTENSION_LIST} xpp::sync::extension)
endif()
if(WITH_XCOMPOSITE)
  set(XCB_PROTOS "${XCB_PROTOS}" composite)
  set(XPP_EXTENSION_LIST ${XPP_EXTENSION_LIST} xpp::composite::extension)
endif()
if(WITH_XKB)
  set(XCB_PROTOS "${XCB_PROTOS}" xkb)
  set(XPP_EXTENSION_LIST ${XPP_EXTENSION_LIST} xpp::xkb::extension)
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
endif()

# }}}
# Optional dependency: libmpdclient {{{

if(ENABLE_MPD)
  find_package(LibMPDClient REQUIRED)
  set(APP_LIBRARIES ${APP_LIBRARIES} ${LIBMPDCLIENT_LIBRARY})
  set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${LIBMPDCLIENT_INCLUDE_DIR})
endif()

# }}}
# Optional dependency: libiw {{{

if(ENABLE_NETWORK)
  find_package(Libiw REQUIRED)
  set(APP_LIBRARIES ${APP_LIBRARIES} ${LIBIW_LIBRARY})
  set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${LIBIW_INCLUDE_DIR})
endif()

# }}}
# Optional dependency: i3ipcpp {{{

if(ENABLE_I3)
  add_subdirectory(${PROJECT_SOURCE_DIR}/lib/i3ipcpp ${PROJECT_BINARY_DIR}/lib/i3ipcpp)
  set(APP_LIBRARIES ${APP_LIBRARIES} ${I3IPCPP_LIBRARIES})
  set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} ${I3IPCPP_INCLUDE_DIRS})
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

target_compile_definitions(${PROJECT_NAME} PUBLIC
  ${X11_XCB_DEFINITIONS}
  ${XCB_DEFINITIONS})

# }}}
# Export target details {{{

set(APP_BINARY ${PROJECT_SOURCE_DIR}/bin/${PROJECT_NAME} PARENT_SCOPE)
set(APP_LIBRARIES ${APP_LIBRARIES} PARENT_SCOPE)
set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} PARENT_SCOPE)

# }}}

execute_process(COMMAND git describe --tags --dirty=-git
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE APP_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

configure_file(
  ${PROJECT_SOURCE_DIR}/include/config.hpp.cmake
  ${CMAKE_SOURCE_DIR}/include/config.hpp
  ESCAPE_QUOTES @ONLY)
