function(libinotify)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(PC_LibInotify QUIET libinotify)

  include(FindPackageHandleStandardArgs)

  find_path(LibInotify_INCLUDES
    NAMES netlink/version.h
    HINTS ${PC_LibInotify_INCLUDEDIR} ${PC_LibInotify_INCLUDE_DIRS}
    )

  find_package_handle_standard_args(LibInotify
    REQUIRED_VARS PC_LibInotify_INCLUDE_DIRS
    VERSION_VAR PC_LibInotify_VERSION
    )

  if(LibInotify_FOUND AND NOT TARGET LibInotify::LibInotify)
    add_library(LibInotify::LibInotify INTERFACE IMPORTED)
    set_target_properties(LibInotify::LibInotify PROPERTIES
      INTERFACE_LINK_LIBRARIES "${PC_LibInotify_LIBRARIES}")

    target_include_directories(LibInotify::LibInotify SYSTEM INTERFACE ${LibInotify_INCLUDES})
  endif()
endfunction()

libinotify()
