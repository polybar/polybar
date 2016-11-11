#
# Generate configuration file
#

set(MODULES_LEFT "bspwm i3 mpd")
set(MODULES_CENTER "")
set(MODULES_RIGHT "backlight volume memory cpu wlan eth battery temperature date powermenu")

# Strip disabled modules {{{

if(NOT ENABLE_ALSA)
  string(REPLACE " volume" "" MODULES_RIGHT ${MODULES_RIGHT})
endif()
if(NOT ENABLE_I3)
  string(REPLACE " i3" "" MODULES_LEFT ${MODULES_LEFT})
endif()
if(NOT ENABLE_MPD)
  string(REPLACE " mpd" "" MODULES_LEFT ${MODULES_LEFT})
endif()
if(NOT ENABLE_NETWORK)
  string(REPLACE " wlan eth" "" MODULES_RIGHT ${MODULES_RIGHT})
endif()
if(NOT ENABLE_RANDR_EXT)
  string(REPLACE "backlight " "" MODULES_RIGHT ${MODULES_RIGHT})
endif()

# }}}
# Get battery/adapter name {{{

string(REGEX REPLACE /%battery%.* "" PATH_BAT ${SETTING_PATH_BATTERY_CAPACITY})
string(REGEX REPLACE /%adapter%.* "" PATH_ADP ${SETTING_PATH_ADAPTER_STATUS})

file(GLOB BAT_LIST RELATIVE ${PATH_BAT} ${PATH_ADP}/B*)
file(GLOB ADP_LIST RELATIVE ${PATH_ADP} ${PATH_ADP}/A*)

list(GET BAT_LIST 0 BATTERY)
list(GET ADP_LIST 0 ADAPTER)

# }}}
# Configure and install {{{

configure_file(
  ${PROJECT_SOURCE_DIR}/examples/config.cmake
  ${CMAKE_SOURCE_DIR}/examples/config
  ESCAPE_QUOTES @ONLY)

install(FILES config
  DESTINATION share/examples/lemonbuddy
  COMPONENT config)

# }}}
