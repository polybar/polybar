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
  DESTINATION share/doc/polybar
  COMPONENT config)

# }}}

# Documentation {{{
configure_file(conf.py.in conf.py @ONLY)

set(SPHINX_FLAGS "" CACHE STRING "Flags to pass to sphinx-build")
separate_arguments(sphinx_flags UNIX_COMMAND "${SPHINX_FLAGS}")

set(doc_path "${CMAKE_CURRENT_SOURCE_DIR}")

# We want to run `sphinx-build` with the following builders
set(doc_builders "html" "man")

# Name of all documentation targets
set(doc_targets "")

foreach(builder ${doc_builders})
  set(doc_target "doc_${builder}")
  set(builder_log "builder-${builder}.log")
  add_custom_target(${doc_target}
    COMMAND sphinx-build
            -b ${builder}
            -c "${CMAKE_CURRENT_BINARY_DIR}"
            -d "${CMAKE_CURRENT_BINARY_DIR}/doctrees"
            ${sphinx_flags}
            "${CMAKE_SOURCE_DIR}/doc"
            "${CMAKE_CURRENT_BINARY_DIR}/${builder}" > ${builder_log}
    COMMENT "sphinx-build ${builder}: see doc/${builder_log}")

  list(APPEND doc_targets ${doc_target})
endforeach()

# Dummy target that depends on all documentation targets
add_custom_target(doc ALL DEPENDS ${doc_targets})

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
  DESTINATION share/doc/polybar
  COMPONENT doc)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/polybar.1
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1
  COMPONENT doc)
# }}}
