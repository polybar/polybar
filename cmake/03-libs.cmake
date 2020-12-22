#
# Check libraries
#

if (BUILD_DOC)
  find_program(BIN_SPHINX "${SPHINX_BUILD}")

  if (NOT BIN_SPHINX)
    message(FATAL_ERROR "sphinx-build executable '${SPHINX_BUILD}' not found.")
  endif()
endif()

if (BUILD_LIBPOLY)
  find_package(Threads REQUIRED)
  find_package(CairoFC REQUIRED)

  if (ENABLE_ALSA)
    find_package(ALSA REQUIRED)
    set(ALSA_VERSION ${ALSA_VERSION_STRING})
  endif()

  if (ENABLE_CURL)
    find_package(CURL REQUIRED)
    set(CURL_VERSION ${CURL_VERSION_STRING})
  endif()

  if (ENABLE_MPD)
    find_package(LibMPDClient REQUIRED)
    set(MPD_VERSION ${LibMPDClient_VERSION})
  endif()

  if (ENABLE_NETWORK)
    if(WITH_LIBNL)
      find_package(LibNlGenl3 REQUIRED)
      set(NETWORK_LIBRARY_VERSION ${LibNlGenl3_VERSION})
    else()
      find_package(Libiw REQUIRED)
    endif()
  endif()

  if (ENABLE_PULSEAUDIO)
    find_package(LibPulse REQUIRED)
    set(PULSEAUDIO_VERSION ${LibPulse_VERSION})
  endif()

  # xcomposite is required
  list(APPEND XORG_EXTENSIONS COMPOSITE)
  if (WITH_XKB)
    list(APPEND XORG_EXTENSIONS XKB)
  endif()
  if (WITH_XCURSOR)
    list(APPEND XORG_EXTENSIONS CURSOR)
  endif()
  if (WITH_XRM)
    list(APPEND XORG_EXTENSIONS XRM)
  endif()

  # Set min xrandr version required
  if (WITH_XRANDR_MONITORS)
    set(XRANDR_VERSION "1.12")
  else ()
    set(XRANDR_VERSION "")
  endif()

  # Randr is required
  find_package(Xcb ${XRANDR_VERSION} REQUIRED COMPONENTS RANDR)
  find_package(Xcb REQUIRED COMPONENTS ${XORG_EXTENSIONS})

  # FreeBSD Support
  if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    find_package(LibInotify REQUIRED)
  endif()
endif()
