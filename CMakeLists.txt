#
# Build configuration
#
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(lemonbuddy CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/modules)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wno-unused-parameter -Wno-unused-local-typedefs")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -O0 -g2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type specified; using Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

include(cmake/utils.cmake)
include(cmake/clang-cpp-tools.cmake)

# Figure out default option values {{{

find_package(ALSA QUIET)
find_package(Libiw QUIET)
find_package(LibMPDClient QUIET)
find_program(CCACHE_BINARY ccache)
find_program(I3_BINARY i3)
find_package(CppUnit QUIET)
if(CPPUNIT_FOUND)
  set(BUILD_TESTS ON)
endif()
if(CCACHE_BINARY)
  set(CCACHE_FOUND ON)
endif()
if(I3_BINARY)
  set(I3_FOUND ON)
endif()

# }}}
# Project settings {{{

option(BUILD_TESTS    "Build testsuite"        ${BUILD_TESTS})
option(ENABLE_CCACHE  "Enable ccache support"  ${CCACHE_FOUND})
option(ENABLE_ALSA    "Enable alsa support"    ${ALSA_FOUND})
option(ENABLE_I3      "Enable i3 support"      ${I3_FOUND})
option(ENABLE_MPD     "Enable mpd support"     ${LIBMPDCLIENT_FOUND})
option(ENABLE_NETWORK "Enable network support" ${LIBIW_FOUND})

if(ENABLE_ALSA)
  set(SETTING_ALSA_SOUNDCARD "default"
    CACHE STRING "Name of the ALSA soundcard driver")
endif()
set(SETTING_CONNECTION_TEST_IP "8.8.8.8"
  CACHE STRING "Address to ping when testing network connection")
set(SETTING_PATH_BACKLIGHT_VAL "/sys/class/backlight/%card%/brightness"
  CACHE STRING "Path to file containing the current backlight value")
set(SETTING_PATH_BACKLIGHT_MAX "/sys/class/backlight/%card%/max_brightness"
  CACHE STRING "Path to file containing the maximum backlight value")
set(SETTING_PATH_BATTERY_CAPACITY "/sys/class/power_supply/%battery%/capacity"
  CACHE STRING "Path to file containing the current battery capacity")
set(SETTING_PATH_ADAPTER_STATUS "/sys/class/power_supply/%adapter%/online"
  CACHE STRING "Path to file containing the current adapter status")
set(SETTING_BSPWM_SOCKET_PATH "/tmp/bspwm_0_0-socket"
  CACHE STRING "Path to bspwm socket")
set(SETTING_BSPWM_STATUS_PREFIX "W"
  CACHE STRING "Prefix prepended to the bspwm status line")
set(SETTING_PATH_CPU_INFO "/proc/stat"
  CACHE STRING "Path to file containing cpu info")
set(SETTING_PATH_MEMORY_INFO "/proc/meminfo"
  CACHE STRING "Path to file containing memory info")

if(ENABLE_CCACHE)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "ccache")
endif()

# }}}
# Locate dependencies {{{

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(Freetype REQUIRED Freetype2)
find_package(PkgConfig)
find_package(X11 REQUIRED COMPONENTS Xft)
find_package(X11_XCB REQUIRED)

pkg_check_modules(FONTCONFIG REQUIRED fontconfig)

link_libraries(${X11_Xft_LIB})
link_libraries(${X11_XCB_LIB})
link_libraries(${BOOST_LIBRARIES})
link_libraries(${CMAKE_THREAD_LIBS_INIT})
link_libraries(${X11_LIBRARIES})
link_libraries(${FREETYPE_LIBRARIES})
link_libraries(${FONTCONFIG_LIBRARIES})
link_libraries()

include_directories(
  ${BOOST_INCLUDE_DIR}
  ${FONTCONFIG_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/lib/boost/include
  ${PROJECT_SOURCE_DIR}/lib/fastdelegate/include)

set(XCB_PROTOS xproto randr)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/xpp)
link_libraries(${XPP_LIBRARIES})

if(ENABLE_ALSA)
  find_package(ALSA REQUIRED)
  link_libraries(${ALSA_LIBRARY})
endif()
if(ENABLE_MPD)
  find_package(LibMPDClient REQUIRED)
  link_libraries(${LIBMPDCLIENT_LIBRARIES})
endif()
if(ENABLE_NETWORK)
  find_package(Libiw REQUIRED)
  link_libraries(${LIBIW_LIBRARY})
endif()
if(ENABLE_I3)
  add_subdirectory(${PROJECT_SOURCE_DIR}/lib/i3ipcpp EXCLUDE_FROM_ALL)
  include_directories(${I3IPCPP_INCLUDE_DIRS})
  link_libraries(${I3IPCPP_LIBRARIES})
endif()

# }}}
# Build source tree {{{

add_subdirectory(${PROJECT_SOURCE_DIR}/man)
add_subdirectory(${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/bin)
add_subdirectory(${PROJECT_SOURCE_DIR}/examples ${PROJECT_BINARY_DIR}/examples)
if(BUILD_TESTS)
  add_subdirectory(${PROJECT_SOURCE_DIR}/tests ${PROJECT_BINARY_DIR}/tests EXCLUDE_FROM_ALL)
endif()

# }}}
# Build summary {{{

message(STATUS "---------------------------")
message(STATUS " Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS " Compiler C: ${CMAKE_C_COMPILER}")
message(STATUS " Compiler C++: ${CMAKE_CXX_COMPILER}")
message(STATUS " Compiler flags: ${CMAKE_CXX_FLAGS}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS " + debug flags:: ${CMAKE_CXX_FLAGS_DEBUG}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS " + release flags:: ${CMAKE_CXX_FLAGS_RELEASE}")
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  message(STATUS " + minsizerel flags:: ${CMAKE_CXX_FLAGS_MINSIZEREL}")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  message(STATUS " + relwithdebinfo flags:: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()
message(STATUS "---------------------------")

message(STATUS " Build testsuite        ${BUILD_TESTS}")
message(STATUS " Enable ccache support  ${ENABLE_CCACHE}")
message(STATUS " Enable alsa support    ${ENABLE_ALSA}")
message(STATUS " Enable i3 support      ${ENABLE_I3}")
message(STATUS " Enable mpd support     ${ENABLE_MPD}")
message(STATUS " Enable network support ${ENABLE_NETWORK}")

if(DISABLE_MODULES)
  message(STATUS " Disable modules        ON")
endif()
if(DISABLE_TRAY)
  message(STATUS " Disable systray        ON")
endif()
if(DISABLE_DRAW)
  message(STATUS " Disable drawing        ON")
endif()

message(STATUS "---------------------------")

# }}}
# Uninstall target {{{

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/uninstall.cmake.in
  ${PROJECT_BINARY_DIR}/cmake/uninstall.cmake
  IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND}
  -P ${PROJECT_BINARY_DIR}/cmake/uninstall.cmake)

# }}}
