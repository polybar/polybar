find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LibNlGenl3 QUIET libnl-genl-3.0)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibNlGenl3
  REQUIRED_VARS PC_LibNlGenl3_INCLUDE_DIRS
  VERSION_VAR PC_LibNlGenl3_VERSION
)

if(LibNlGenl3_FOUND AND NOT TARGET LibNlGenl3::LibNlGenl3)
  add_library(LibNlGenl3::LibNlGenl3 INTERFACE IMPORTED)

  set_target_properties(LibNlGenl3::LibNlGenl3 PROPERTIES
    INTERFACE_LINK_LIBRARIES "${PC_LibNlGenl3_LIBRARIES}")
  target_include_directories(LibNlGenl3::LibNlGenl3 SYSTEM INTERFACE ${PC_LibNlGenl3_INCLUDE_DIRS})
endif()
