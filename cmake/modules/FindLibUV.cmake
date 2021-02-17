# This module defines
# LIBUV_FOUND
# LIBUV_INCLUDE_DIR
# LIBUV_INCLUDE_DIRS
# LIBUV_LIBRARY
# LIBUV_LIBRARIES
# LIBUV_VERSION

find_package_impl("libuv" "LIBUV" "")

if(LIBUV_FOUND AND NOT TARGET LibUV::LibUV)
  create_imported_target("LibUV::LibUV" "${LIBUV_INCLUDE_DIR}" "${LIBUV_LIBRARY}")
endif()
