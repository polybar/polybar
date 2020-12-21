# This module defines an imported target `LibMPDClient::LibMPDClient` if libmpdclient is found
#
# Defines the following Variables (see find_package_impl for more info):
# LibMPDClient_FOUND
# LibMPDClient_INCLUDE_DIR
# LibMPDClient_INCLUDE_DIRS
# LibMPDClient_LIBRARY
# LibMPDClient_LIBRARIES
# LibMPDClient_VERSION
find_package_impl("libmpdclient" "LibMPDClient" "mpd/player.h")

if(LibMPDClient_FOUND AND NOT TARGET LibMPDClient::LibMPDClient)
  create_imported_target("LibMPDClient::LibMPDClient" "${LibMPDClient_INCLUDE_DIR}" "${LibMPDClient_LIBRARY}")
endif()
