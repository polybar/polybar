# This module defines
# LibUV_FOUND
# LibUV_INCLUDE_DIR
# LibUV_INCLUDE_DIRS
# LibUV_LIBRARY
# LibUV_LIBRARIES
# LibUV_VERSION

find_package_impl("libuv" "LibUV" "uv.h")

if(LibUV_FOUND AND NOT TARGET LibUV::LibUV)
  create_imported_target("LibUV::LibUV" "${LibUV_INCLUDE_DIR}" "${LibUV_LIBRARY}")
endif()
