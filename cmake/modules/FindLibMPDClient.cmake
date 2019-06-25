find_package_impl("libmpdclient" "LibMPDClient" "mpd/player.h")

if(LibMPDClient_FOUND AND NOT TARGET LibMPDClient::LibMPDClient)
  create_imported_target("LibMPDClient::LibMPDClient" "${LibMPDClient_INCLUDE_DIR}" "${LibMPDClient_LIBRARY}")
endif()
