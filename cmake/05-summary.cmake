#
# Output build summary
#

message(STATUS " Build:")
message_colored(STATUS "   Version: ${APP_VERSION}" "32;1")
message_colored(STATUS "   Type: ${CMAKE_BUILD_TYPE}" "37;2")
if (HAS_CXX_COMPILATION)
  message_colored(STATUS "   CXX: ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}" "37;2")
  message_colored(STATUS "   LD: ${CMAKE_LINKER} ${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}" "37;2")
endif()
if (BUILD_DOC)
  message_colored(STATUS "   sphinx-build: ${BIN_SPHINX} ${SPHINX_FLAGS}" "37;2")
endif()

message(STATUS " Install Paths:")
message_colored(STATUS "   PREFIX:  ${CMAKE_INSTALL_PREFIX}" "32")
message_colored(STATUS "   BINDIR:  ${CMAKE_INSTALL_FULL_BINDIR}" "32")
message_colored(STATUS "   DATADIR: ${CMAKE_INSTALL_FULL_DATADIR}" "32")
message_colored(STATUS "   DOCDIR:  ${CMAKE_INSTALL_FULL_DOCDIR}" "32")
message_colored(STATUS "   MANDIR:  ${CMAKE_INSTALL_FULL_MANDIR}" "32")

message(STATUS " Targets:")
colored_option("   polybar" BUILD_POLYBAR)
colored_option("   polybar-msg" BUILD_POLYBAR_MSG)
colored_option("   testsuite" BUILD_TESTS)
colored_option("   documentation" BUILD_DOC)
colored_option("      html" BUILD_DOC_HTML)
colored_option("      man" BUILD_DOC_MAN)

if (BUILD_LIBPOLY)
  message(STATUS " Module support:")
  colored_option("   alsa" ENABLE_ALSA ALSA_VERSION)
  colored_option("   curl" ENABLE_CURL CURL_VERSION)
  colored_option("   i3" ENABLE_I3)
  colored_option("   mpd" ENABLE_MPD MPD_VERSION)
  colored_option("   network (${WIRELESS_LIB})" ENABLE_NETWORK NETWORK_LIBRARY_VERSION)
  colored_option("   pulseaudio" ENABLE_PULSEAUDIO PULSEAUDIO_VERSION)
  colored_option("   xkeyboard" WITH_XKB Xcb_XKB_VERSION)

  message(STATUS " X extensions:")
  colored_option("   xcb-randr" Xcb_RANDR_FOUND Xcb_RANDR_VERSION)
  colored_option("   xcb-randr (monitor support)" WITH_XRANDR_MONITORS Xcb_RANDR_VERSION)
  colored_option("   xcb-composite" Xcb_COMPOSITE_FOUND Xcb_COMPOSITE_VERSION)
  colored_option("   xcb-xkb" WITH_XKB Xcb_XKB_VERSION)
  colored_option("   xcb-xrm" WITH_XRM Xcb_XRM_VERSION)
  colored_option("   xcb-cursor" WITH_XCURSOR Xcb_CURSOR_VERSION)

  message(STATUS " Log options:")
  colored_option("   Trace logging" DEBUG_LOGGER)

  if(CMAKE_BUILD_TYPE_UPPER MATCHES DEBUG)
    message(STATUS " Debug options:")
    colored_option("   Trace logging (verbose)" DEBUG_LOGGER_VERBOSE)
    colored_option("   Draw clickable areas" DEBUG_HINTS)
    colored_option("   Print fc-match details" DEBUG_FONTCONFIG)
    colored_option("   Enable window shading" DEBUG_SHADED)
  endif()
endif()
