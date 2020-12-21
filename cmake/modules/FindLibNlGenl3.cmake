# This module defines an imported target `LibNlGenl3::LibNlGenl3` if libnl-genl-3.0 is found
#
# Defines the following Variables (see find_package_impl for more info):
# LibNlGenl3_FOUND
# LibNlGenl3_INCLUDE_DIR
# LibNlGenl3_INCLUDE_DIRS
# LibNlGenl3_LIBRARY
# LibNlGenl3_LIBRARIES
# LibNlGenl3_VERSION
find_package_impl("libnl-genl-3.0" "LibNlGenl3" "")

if(LibNlGenl3_FOUND AND NOT TARGET LibNlGenl3::LibNlGenl3)
  create_imported_target("LibNlGenl3::LibNlGenl3" "${LibNlGenl3_INCLUDE_DIR}" "${LibNlGenl3_LIBRARY}")
endif()
