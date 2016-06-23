#
# Build configuration
#
cmake_minimum_required(VERSION 3.0)

# TODO(jaagr): Replace with git tag
project(lemonbuddy VERSION 1.2.2)

# Include the local cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wpedantic -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -g3 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wno-unused-variable")

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type specified; using Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

string(ASCII 27 ANSI)

#
# Internal values and switches
#

# Keep track if the i3 option is explicitly defined
if(ENABLE_I3)
  set(ENABLE_I3_NODEF ON)
endif()

option(ENABLE_CCACHE  "Enable ccache support"  ON)
option(ENABLE_ALSA    "Enable alsa support"    ON)
option(ENABLE_I3      "Enable i3 support"      ON)
option(ENABLE_MPD     "Enable mpd support"     ON)
option(ENABLE_NETWORK "Enable network support" ON)

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
  find_program(CCACHE_FOUND "ccache")
  if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "ccache")
  endif()
endif()

#
# Locate and insert libs
#
find_package("Boost" REQUIRED)
find_package("Threads" REQUIRED)

set(PROJECT_INCL_DIRS "${PROJECT_SOURCE_DIR}/include"
  ${BOOST_INCLUDE_DIR})
set(PROJECT_LINK_LIBS
  ${BOOST_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})

if(ENABLE_I3)
  find_program(I3_EXECUTABLE "i3")
  if(I3_EXECUTABLE)
    add_subdirectory("${PROJECT_SOURCE_DIR}/contrib/i3ipcpp" EXCLUDE_FROM_ALL)
    set(PROJECT_INCL_DIRS
      ${PROJECT_INCL_DIRS}
      ${SIGCPP_INCLUDE_DIRS}
      ${I3IPCPP_INCLUDE_DIRS})
    set(PROJECT_LINK_LIBS
      ${PROJECT_LINK_LIBS}
      ${I3IPCPP_LIBRARIES})
  else(I3_EXECUTABLE)
    if(NOT ENABLE_I3_NODEF)
      message(WARNING "${ANSI}[41;1mDisabling \"i3 module\" support (prerequisites failed)${ANSI}[0m")
      set(ENABLE_I3 OFF)
    endif()
  endif()
endif()

if(ENABLE_ALSA)
  find_package("ALSA")
  if(ALSA_FOUND)
    set(PROJECT_INCL_DIRS ${PROJECT_INCL_DIRS} ${ALSA_INCLUDE_DIR})
    set(PROJECT_LINK_LIBS ${PROJECT_LINK_LIBS} ${ALSA_LIBRARY})
  else(ALSA_FOUND)
    message(WARNING "${ANSI}[41;1mDisabling \"volume module\" support (prerequisites failed)${ANSI}[0m")
    set(ENABLE_ALSA OFF)
  endif()
endif()

if(ENABLE_MPD)
  find_package("LibMPDClient")
  if(LIBMPDCLIENT_FOUND)
    set(PROJECT_INCL_DIRS ${PROJECT_INCL_DIRS} ${LIBMPDCLIENT_INCLUDE_DIR})
    set(PROJECT_LINK_LIBS ${PROJECT_LINK_LIBS} ${LIBMPDCLIENT_LIBRARY})
  else(LIBMPDCLIENT_FOUND)
    message(WARNING "${ANSI}[41;1mDisabling \"mpd module\" support (prerequisites failed)${ANSI}[0m")
    set(ENABLE_MPD OFF)
  endif()
endif()

if(ENABLE_NETWORK)
  find_package("Libiw")
  if(LIBIW_FOUND)
    set(PROJECT_INCL_DIRS ${PROJECT_INCL_DIRS} ${LIBIW_INCLUDE_DIR})
    set(PROJECT_LINK_LIBS ${PROJECT_LINK_LIBS} ${LIBIW_LIBRARY})
  else(LIBIW_FOUND)
    message(WARNING "${ANSI}[41;1mDisabling \"network module\" support (prerequisites failed)${ANSI}[0m")
    set(ENABLE_NETWORK OFF)
  endif()
endif()

#
# Load the xpp library
#
set(XCB_PROTOS xproto randr)
add_subdirectory("${PROJECT_SOURCE_DIR}/contrib/xpp")
set(PROJECT_INCL_DIRS ${PROJECT_INCL_DIRS} ${XPP_INCLUDE_DIRS})
set(PROJECT_LINK_LIBS ${PROJECT_LINK_LIBS} ${XPP_LIBRARIES})

#
# Install executable and wrapper
#
message(STATUS "---------------------------")
message(STATUS " Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS " Compiler C: ${CMAKE_C_COMPILER}")
message(STATUS " Compiler C++: ${CMAKE_CXX_COMPILER}")
message(STATUS "---------------------------")
message(STATUS " Enable ccache support  ${ENABLE_CCACHE}")
message(STATUS " Enable alsa support    ${ENABLE_ALSA}")
message(STATUS " Enable i3 support      ${ENABLE_I3}")
message(STATUS " Enable mpd support     ${ENABLE_MPD}")
message(STATUS " Enable network support ${ENABLE_NETWORK}")
message(STATUS "---------------------------")

add_subdirectory("${PROJECT_SOURCE_DIR}/src" EXCLUDE_FROM_ALL)
link_directories(${PROJECT_LINK_DIRS})
include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR} ${PROJECT_INCL_DIRS})
link_libraries(${PROJECT_LINK_LIBS})
add_executable(${PROJECT_NAME} ${FILES}
  "examples/config"
  "examples/config.bspwm"
  "examples/config.i3")
target_link_libraries(${PROJECT_NAME} ${PROJECT_LINK_LIBS})

configure_file("${CMAKE_SOURCE_DIR}/include/config.hpp.cmake" "${CMAKE_SOURCE_DIR}/include/config.hpp" ESCAPE_QUOTES @ONLY)

install(TARGETS ${PROJECT_NAME}
  DESTINATION "bin"
  COMPONENT "binaries")
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/lemonbuddy_wrapper"
  DESTINATION "bin" COMPONENT "binaries")
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/lemonbuddy_terminate"
  DESTINATION "bin" COMPONENT "binaries")

install(FILES "examples/config"
  DESTINATION "share/examples/${PROJECT_NAME}"
  COMPONENT "config")
install(FILES "examples/config.bspwm"
  DESTINATION "share/examples/${PROJECT_NAME}"
  COMPONENT "config")
install(FILES "examples/config.i3"
  DESTINATION "share/examples/${PROJECT_NAME}"
  COMPONENT "config")

#
# Uninstall target
#
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake"
  IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND}
  -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake")
