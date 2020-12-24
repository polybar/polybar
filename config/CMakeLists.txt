#
# Generate configuration file
#

set(MODULES_LEFT "bspwm;i3")
set(MODULES_CENTER "mpd")
set(MODULES_RIGHT "filesystem;backlight-acpi;alsa;pulseaudio;xkeyboard;memory;cpu;wlan;eth;battery;temperature;date;powermenu")

set(FONT_FIXED "fixed:pixelsize=10")
set(FONT_UNIFONT "unifont:fontformat=truetype")
set(FONT_SIJI "siji:pixelsize=10")

queryfont(FONT_FIXED ${FONT_FIXED} FIELDS family pixelsize)
queryfont(FONT_UNIFONT ${FONT_UNIFONT} FIELDS family fontformat)
queryfont(FONT_SIJI ${FONT_SIJI} FIELDS family pixelsize)

# Strip disabled modules {{{

if(NOT ENABLE_PULSEAUDIO)
  list(REMOVE_ITEM MODULES_RIGHT "pulseaudio")
endif()
if(NOT ENABLE_ALSA)
  list(REMOVE_ITEM MODULES_RIGHT "alsa")
endif()
if(NOT ENABLE_I3)
  list(REMOVE_ITEM MODULES_LEFT "i3")
endif()
if(NOT ENABLE_MPD)
  list(REMOVE_ITEM MODULES_CENTER "mpd")
endif()
if(NOT ENABLE_NETWORK)
  list(REMOVE_ITEM MODULES_RIGHT "wlan" "eth")
endif()
if(NOT WITH_XKB)
  list(REMOVE_ITEM MODULES_RIGHT "xkeyboard")
endif()

string(REPLACE ";" " " MODULES_LEFT "${MODULES_LEFT}")
string(REPLACE ";" " " MODULES_CENTER "${MODULES_CENTER}")
string(REPLACE ";" " " MODULES_RIGHT "${MODULES_RIGHT}")

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
  ${CMAKE_BINARY_DIR}/generated-sources/config
  ESCAPE_QUOTES @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/generated-sources/config
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
  COMPONENT config)

# }}}
# Target: userconfig {{{

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/templates/userconfig.cmake.in
  ${PROJECT_BINARY_DIR}/cmake/userconfig.cmake
  ESCAPE_QUOTES @ONLY)

add_custom_target(userconfig
  COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/cmake/userconfig.cmake)

# }}}
