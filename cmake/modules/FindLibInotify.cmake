find_package_impl("libinotify" "LibInotify" "")

if(LibInotify_FOUND AND NOT TARGET LibInotify::LibInotify)
  create_imported_target("LibInotify::LibInotify" "${LibInotify_INCLUDE_DIR}" "${LibInotify_LIBRARY}")
endif()
