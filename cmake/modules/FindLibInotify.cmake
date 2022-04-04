# This module defines an imported target `LibInotify::LibInotify` if libinotify is found
#
# Defines the following Variables (see find_package_impl for more info):
# LibInotify_FOUND
# LibInotify_INCLUDE_DIR
# LibInotify_INCLUDE_DIRS
# LibInotify_LIBRARY
# LibInotify_LIBRARIES
# LibInotify_VERSION
find_package_impl("libinotify" "LibInotify" "")

if(LibInotify_FOUND AND NOT TARGET LibInotify::LibInotify)
  create_imported_target("LibInotify::LibInotify" "${LibInotify_INCLUDE_DIR}" "${LibInotify_LIBRARY}")
endif()
