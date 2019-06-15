#
# Check libraries
#

find_package(Threads REQUIRED)
find_package(CairoFC REQUIRED)

if (ENABLE_ALSA)
  find_package(ALSA REQUIRED)
endif()

if (ENABLE_CURL)
  find_package(CURL REQUIRED)
endif()

if (ENABLE_MPD)
  find_package(LibMPDClient REQUIRED)
endif()

if (ENABLE_NETWORK)
  if(WITH_LIBNL)
    find_package(LibNlGenl3 REQUIRED)
  else()
    find_package(Libiw REQUIRED)
  endif()
endif()

if (ENABLE_PULSEAUDIO)
  find_package(LibPulse REQUIRED)
endif()

# Randr is required
set(XORG_EXTENSIONS RANDR)
if (WITH_XCOMPOSITE)
  set(XORG_EXTENSIONS ${XORG_EXTENSIONS} COMPOSITE)
endif()
if (WITH_XKB)
  set(XORG_EXTENSIONS ${XORG_EXTENSIONS} XKB)
endif()
if (WITH_XCURSOR)
  set(XORG_EXTENSIONS ${XORG_EXTENSIONS} CURSOR)
endif()
if (WITH_XRM)
  set(XORG_EXTENSIONS ${XORG_EXTENSIONS} XRM)
endif()

set(XCB_VERSION "")
if (WITH_XRANDR_MONITOR)
  set(XCB_VERSION "1.12")
endif()

find_package(Xcb ${XCB_VERSION} REQUIRED COMPONENTS ${XORG_EXTENSIONS})

# FreeBSD Support
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  find_package(LibInotify REQUIRED)
endif()
