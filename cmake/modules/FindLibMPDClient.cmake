find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LibMPDClient QUIET libmpdclient)

include(FindPackageHandleStandardArgs)

find_path(LibMPDClient_INCLUDES
  NAMES mpd/player.h
  HINTS ${PC_LibMPDClient_INCLUDEDIR} ${PC_LibMPDClient_INCLUDE_DIRS}
  )

find_package_handle_standard_args(LibMPDClient
  REQUIRED_VARS LibMPDClient_INCLUDES
  VERSION_VAR PC_LibMPDClient_VERSION
  )

if(LibMPDClient_FOUND AND NOT TARGET LibMPDClient::LibMPDClient)
  add_library(LibMPDClient::LibMPDClient INTERFACE IMPORTED)
  set_target_properties(LibMPDClient::LibMPDClient PROPERTIES
    INTERFACE_LINK_LIBRARIES "${PC_LibMPDClient_LIBRARIES}")

  target_include_directories(LibMPDClient::LibMPDClient SYSTEM INTERFACE ${LibMPDClient_INCLUDES})
endif()
