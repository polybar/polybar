# - Try to find LibMPDClient
# Once done, this will define
#
#   LIBMPDCLIENT_FOUND - System has LibMPDClient
#   LIBMPDCLIENT_INCLUDE_DIRS - The LibMPDClient include directories
#   LIBMPDCLIENT_LIBRARIES - The libraries needed to use LibMPDClient
#   LIBMPDCLIENT_DEFINITIONS - Compiler switches required for using LibMPDClient

find_package(PkgConfig)
pkg_check_modules(PC_LIBMPDCLIENT QUIET libmpdclient)
set(LIBMPDCLIENT_DEFINITIONS ${PC_LIBMPDCLIENT_CFLAGS_OTHER})

find_path(LIBMPDCLIENT_INCLUDE_DIR
    NAMES mpd/player.h
    HINTS ${PC_LIBMPDCLIENT_INCLUDEDIR} ${PC_LIBMPDCLIENT_INCLUDE_DIRS}
)

find_library(LIBMPDCLIENT_LIBRARY
    NAMES mpdclient
    HINTS ${PC_LIBMPDCLIENT_LIBDIR} ${PC_LIBMPDCLIENT_LIBRARY_DIRS}
)

set(LIBMPDCLIENT_LIBRARIES ${LIBMPDCLIENT_LIBRARY})
set(LIBMPDCLIENT_INCLUDE_DIRS ${LIBMPDCLIENT_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibMPDClient DEFAULT_MSG
    LIBMPDCLIENT_LIBRARY LIBMPDCLIENT_INCLUDE_DIR
)

mark_as_advanced(LIBMPDCLIENT_LIBRARY LIBMPDCLIENT_INCLUDE_DIR)
