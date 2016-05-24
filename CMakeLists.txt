#
# Build configuration
#
cmake_minimum_required(VERSION 3.0)

if(NOT CMAKE_CXX_COMPILER)
  find_program(clang_EXECUTABLE "clang++")
  if(clang_EXECUTABLE)
    set(CMAKE_CXX_COMPILER ${clang_EXECUTABLE})
  endif()
endif()

project(lemonbuddy VERSION 1.0.0)

set(CMAKE_MODULE_PATH
  "${CMAKE_MODULE_PATH}"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wpedantic -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -g3 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type specified; using Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

#
# Internal values and switches
#
option(ENABLE_CCACHE  "Enable ccache support"  ON)
option(ENABLE_ALSA    "Enable alsa support"    ON)
option(ENABLE_I3      "Enable i3 support"      ON)
option(ENABLE_MPD     "Enable mpd support"     ON)
option(ENABLE_NETWORK "Enable network support" ON)

if(ENABLE_ALSA)
  set(SETTING_ALSA_SOUNDCARD "default"
    CACHE STRING "Name of the ALSA soundcard driver")
endif()

if(ENABLE_MPD)
  set(SETTING_MPD_HOST "127.0.0.1"
    CACHE STRING "Address MPD is bound to")
  set(SETTING_MPD_PASSWORD ""
    CACHE STRING "Password required for authentication")
  set(SETTING_MPD_PORT "6600"
    CACHE STRING "Port MPD is bound to")
endif()

set(SETTING_CONNECTION_TEST_IP "8.8.8.8"
  CACHE STRING "Address to ping when testing network connection")
set(SETTING_PATH_BACKLIGHT_VAL "/sys/class/backlight/%card%/brightness"
  CACHE STRING "Path to file containing the current backlight value")
set(SETTING_PATH_BACKLIGHT_MAX "/sys/class/backlight/%card%/max_brightness"
  CACHE STRING "Path to file containing the maximum backlight value")
set(SETTING_PATH_BATTERY_WATCH "/sys/class/power_supply/%battery%/charge_now"
  CACHE STRING "Path to attach inotify watch to")
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
  else()
    message(WARNING "ccache not found. Ignoring the flag...")
  endif()
endif()

#
# Locate and insert libs
#
find_package("Boost" REQUIRED)
find_package("Threads" REQUIRED)
find_package("X11" REQUIRED "X11_Xrandr" "X11_Xutil" "X11_Xlib")

include_directories(
  "${PROJECT_SOURCE_DIR}/include"
  ${Boost_INCLUDE_DIRS}
  ${X11_INCLUDE_DIR}
  ${X11_Xrandr_INCLUDE_PATH})

link_directories(
  ${Boost_LIBRARY_DIRS}
  ${X11_LIBRARY_DIRS})

set(LINK_LIBS
  ${Boost_LIBRARIES}
  ${X11_LIBRARIES}
  ${X11_Xrandr_LIB}
  ${CMAKE_THREAD_LIBS_INIT})

if(ENABLE_ALSA)
  find_package("ALSA")
  if(ALSA_FOUND)
    include_directories(${ALSA_INCLUDE_DIRS})
    link_directories(${ALSA_LIBRARY_DIRS})
    set(LINK_LIBS ${LINK_LIBS} ${ALSA_LIBRARIES})
  else(ALSA_FOUND)
    # graceful fail
    message(WARNING "Disabling alsa since required package wasn't found")
    set(ENABLE_ALSA OFF)
  endif()
endif()

if(ENABLE_I3)
  find_program(I3_EXECUTABLE "i3")
  if(NOT I3_EXECUTABLE)
    # graceful fail
    message(WARNING "Disabling i3 support since executable was not found")
    set(ENABLE_ALSA OFF)
  endif()
endif()

if(ENABLE_MPD)
  find_package("LibMPDClient")
  if(LibMPDClient_FOUND)
    include_directories(${LIBMPDCLIENT_INCLUDE_DIRS})
    link_directories(${LIBMPDCLIENT_LIBRARY_DIRS})
    set(LINK_LIBS ${LINK_LIBS} ${LIBMPDCLIENT_LIBRARY})
  else(LibMPDClient_FOUND)
    # graceful fail
    message(WARNING "Disabling mpd since required package wasn't found")
    set(ENABLE_MPD OFF)
  endif()
endif()

if(ENABLE_NETWORK)
  find_package("Libiw")
  if(LibMPDClient_FOUND)
    include_directories(${LIBMPDCLIENT_INCLUDE_DIRS})
    link_directories(${LIBMPDCLIENT_LIBRARY_DIRS})
    set(LINK_LIBS ${LINK_LIBS} ${LIBMPDCLIENT_LIBRARY})
  else(Libiw_FOUND)
    # graceful fail
    message(WARNING "Disabling network support since required library wasn't found")
    set(ENABLE_NETWORK OFF)
  endif()
endif()

#
# Install executable and wrapper
#
message(STATUS "---------------------------")
message(STATUS " Enable ccache support  ${ENABLE_CCACHE}")
message(STATUS " Enable alsa support    ${ENABLE_ALSA}")
message(STATUS " Enable i3 support      ${ENABLE_I3}")
message(STATUS " Enable mpd support     ${ENABLE_MPD}")
message(STATUS " Enable network support ${ENABLE_NETWORK}")
message(STATUS "---------------------------")

add_subdirectory("${PROJECT_SOURCE_DIR}/src")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(ENABLE_I3)
  add_subdirectory("${PROJECT_SOURCE_DIR}/contrib/i3ipcpp" EXCLUDE_FROM_ALL)
  include_directories(${SIGCPP_INCLUDE_DIRS} ${I3IPCpp_INCLUDE_DIRS})
  link_directories(${SIGCPP_LIBRARY_DIRS} ${I3IPCpp_LIBRARY_DIRS})
  set(LINK_LIBS "${LINK_LIBS};${I3IPCpp_LIBRARIES}")
endif()

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})
configure_file("${CMAKE_SOURCE_DIR}/include/config.hpp.cmake" "${CMAKE_SOURCE_DIR}/include/config.hpp" ESCAPE_QUOTES @ONLY)

install(TARGETS ${PROJECT_NAME} DESTINATION "bin")
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/lemonbuddy_wrapper.sh" DESTINATION "bin")

#
# Uninstall target
#
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake"
  IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND}
  -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake")
