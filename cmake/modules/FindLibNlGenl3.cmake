find_package_impl("libnl-genl-3.0" "LibNlGenl3" "")

if(LibNlGenl3_FOUND AND NOT TARGET LibNlGenl3::LibNlGenl3)
  create_imported_target("LibNlGenl3::LibNlGenl3" "${LibNlGenl3_INCLUDE_DIR}" "${LibNlGenl3_LIBRARY}")
endif()
