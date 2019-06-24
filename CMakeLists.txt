#
# Build configuration
#
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# Enable ccache by default and as early as possible because project() performs
# checks on the compiler
option(ENABLE_CCACHE "Enable ccache support" ON)
if(ENABLE_CCACHE)
  message(STATUS "Trying to enable ccache")
  find_program(BIN_CCACHE ccache)

  string(ASCII 27 esc)
  if(NOT BIN_CCACHE)
    message(STATUS "${esc}[33mCouldn't locate ccache, disabling ccache...${esc}[0m")
  else()
    # Enable only if the binary is found
    message(STATUS "${esc}[32mUsing compiler cache ${BIN_CCACHE}${esc}[0m")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${BIN_CCACHE})
  endif()
endif()

project(polybar CXX)

# Extract version information from version.txt. The first line that looks like
# a version string is used, so the file supports comments
file(STRINGS version.txt version_txt REGEX "^[0-9]+\\.[0-9]+\\.[0-9]+.*$" LIMIT_COUNT 1)

# If we are in a git repo we can get the version information from git describe
execute_process(COMMAND git describe --tags --dirty=-dev
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_result
  OUTPUT_VARIABLE git_describe
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

if(git_result EQUAL "0")
  set(APP_VERSION "${git_describe}")
else()
  message(STATUS "Could not detect version with git, falling back to built-in version information.")
  set(APP_VERSION "${version_txt}")
endif()

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${PROJECT_SOURCE_DIR}/cmake
  ${PROJECT_SOURCE_DIR}/cmake/common
  ${PROJECT_SOURCE_DIR}/cmake/modules)

include(GNUInstallDirs)
include(utils)
include(01-core)
include(02-opts)
include(03-libs)
include(04-targets)
include(05-summary)

if(BUILD_DOC)
  add_subdirectory(doc)
endif()
add_subdirectory(contrib/bash)
add_subdirectory(contrib/zsh)
add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(src bin)

# We need to enable testing in the root folder so that 'ctest' and 'make test'
# can be run in the build directory
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()


#
# Generate configuration file
#

set(MODULES_LEFT "bspwm i3")
set(MODULES_CENTER "mpd")
set(MODULES_RIGHT "filesystem xbacklight alsa pulseaudio xkeyboard memory cpu wlan eth battery temperature date powermenu")

set(FONT_FIXED "fixed:pixelsize=10")
set(FONT_UNIFONT "unifont:fontformat=truetype")
set(FONT_SIJI "siji:pixelsize=10")

queryfont(FONT_FIXED ${FONT_FIXED} FIELDS family pixelsize)
queryfont(FONT_UNIFONT ${FONT_UNIFONT} FIELDS family fontformat)
queryfont(FONT_SIJI ${FONT_SIJI} FIELDS family pixelsize)

# Strip disabled modules {{{

if(NOT ENABLE_PULSEAUDIO)
  string(REPLACE " pulseaudio" "" MODULES_RIGHT ${MODULES_RIGHT})
endif()
if(NOT ENABLE_ALSA)
  string(REPLACE " alsa" "" MODULES_RIGHT ${MODULES_RIGHT})
endif()
if(NOT ENABLE_I3)
  string(REPLACE " i3" "" MODULES_LEFT ${MODULES_LEFT})
endif()
if(NOT ENABLE_MPD)
  string(REPLACE "mpd" "" MODULES_CENTER ${MODULES_CENTER})
endif()
if(NOT ENABLE_NETWORK)
  string(REPLACE " wlan eth" "" MODULES_RIGHT ${MODULES_RIGHT})
endif()
if(NOT WITH_XRANDR)
  string(REPLACE "xbacklight " "backlight-acpi " MODULES_RIGHT ${MODULES_RIGHT})
endif()
if(NOT WITH_XKB)
  string(REPLACE "xkeyboard " "" MODULES_RIGHT ${MODULES_RIGHT})
endif()

# }}}
# Get battery/adapter name {{{

string(REGEX REPLACE /%battery%.* "" PATH_BAT ${SETTING_PATH_BATTERY})
string(REGEX REPLACE /%adapter%.* "" PATH_ADP ${SETTING_PATH_ADAPTER})
file(GLOB BAT_LIST RELATIVE ${PATH_BAT} ${PATH_ADP}/B*)
file(GLOB ADP_LIST RELATIVE ${PATH_ADP} ${PATH_ADP}/A*)
if(BAT_LIST)
  list(GET BAT_LIST 0 BATTERY)
else()
  set(BATTERY BAT0)
endif()
if(ADP_LIST)
  list(GET ADP_LIST 0 ADAPTER)
else()
  set(ADAPTER ADP1)
endif()

# }}}
# Get network interfaces {{{

if(ENABLE_NETWORK)
  file(GLOB IFLIST RELATIVE /sys/class/net /sys/class/net/*)
  foreach(INTERFACE ${IFLIST})
    if(NOT ${INTERFACE} STREQUAL "lo")
      file(GLOB IS_WIRELESS /sys/class/net/${INTERFACE}/wireless)
      if(IS_WIRELESS)
        set(INTERFACE_WLAN ${INTERFACE})
      else()
        set(INTERFACE_ETH ${INTERFACE})
      endif()
    endif()
  endforeach()
  if(NOT INTERFACE_ETH)
    set(INTERFACE_ETH net0)
  endif()
  if(NOT INTERFACE_WLAN)
    set(INTERFACE_WLAN net1)
  endif()
endif()

# }}}
# Configure and install {{{

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/config.cmake
  ${CMAKE_CURRENT_LIST_DIR}/config
  ESCAPE_QUOTES @ONLY)

install(FILES config
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
  COMPONENT config)

# }}}
