#
# Build configuration
#
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(lemonbuddy CXX)

# Include local cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/modules)

# Load cmake utility functions
include(cmake/utils.cmake)

# Warn if using an unsupported compiler
if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL Clang)
  message_colored(WARNING "Unsupported compiler!" 31)
endif()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  message_colored(STATUS "No build type specified; using Release" 33)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# Export compile commands used for custom targets
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Generic compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic-errors")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

# Debug specific compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pedantic-errors")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g2")

# Release specific compiler flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# Generic linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++ -lc++abi")

# Project settings {{{

# Set default option values based on current environment
find_package(ALSA QUIET)
find_package(Libiw QUIET)
find_package(LibMPDClient QUIET)
find_program(I3_BINARY i3)
if(I3_BINARY)
  set(I3_FOUND ON)
endif()

option(BUILD_TESTS    "Build testsuite"        OFF)
option(ENABLE_CCACHE  "Enable ccache support"  OFF)
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
  require_binary(ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${BINPATH_ccache})
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${BINPATH_ccache})
endif()

# }}}
# Add subdirectories {{{

add_subdirectory(${PROJECT_SOURCE_DIR}/man)
add_subdirectory(${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/bin)
add_subdirectory(${PROJECT_SOURCE_DIR}/examples ${PROJECT_BINARY_DIR}/examples)

if(BUILD_TESTS)
  add_subdirectory(${PROJECT_SOURCE_DIR}/tests ${PROJECT_BINARY_DIR}/tests)
else()
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

# Custom target: uninstall {{{

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/uninstall.cmake.in
  ${PROJECT_BINARY_DIR}/cmake/uninstall.cmake
  IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND}
  -P ${PROJECT_BINARY_DIR}/cmake/uninstall.cmake)

# }}}
# Custom target: clang-format {{{

find_program(CLANG_FORMAT "clang-format")

if(CLANG_FORMAT)
  file(GLOB_RECURSE HEADERS ${PROJECT_SOURCE_DIR}/include/*.hpp)
  file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

  add_custom_target(clang-format COMMAND ${CLANG_FORMAT}
    -i -style=file ${HEADERS} ${SOURCES})
endif()

find_program(CLANG_TIDY "clang-tidy")

# }}}
# Custom target: clang-tidy {{{

if(CLANG_TIDY)
  add_custom_target(clang-tidy COMMAND ${CLANG_TIDY}
    -p ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/src/main.cpp)
endif()

# }}}
