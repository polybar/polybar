cmake_policy(PUSH)
cmake_policy(SET CMP0057 NEW)

set(XCB_known_components
  XCB
  RANDR
  COMPOSITE
  XKB
  XRM
  CURSOR)

foreach(_comp ${XCB_known_components})
  string(TOLOWER "${_comp}" _lc_comp)
  set(XCB_${_comp}_pkg_config "xcb-${_lc_comp}")
  set(XCB_${_comp}_header "xcb/${_lc_comp}.h")
endforeach()
set(XCB_XRM_header "xcb/xcb_xrm.h")
set(XCB_CURSOR_header "xcb/xcb_cursor.h")

find_package(PkgConfig REQUIRED)
include(FindPackageHandleStandardArgs)

foreach(_comp ${Xcb_FIND_COMPONENTS})
  if (NOT ${_comp} IN_LIST XCB_known_components)
    cmake_policy(POP)
    message(FATAL_ERROR "Unknow component ${_comp} for XCB")
  endif()

  pkg_check_modules(PC_${_comp} QUIET ${XCB_${_comp}_pkg_config})

  find_path(${_comp}_INCLUDES_DIRS
    NAMES "${XCB_${_comp}_header}"
    HINTS ${PC_${_comp}_INCLUDEDIR} ${PC_${_comp}_INCLUDE_DIRS}
  )

  find_package_handle_standard_args(Xcb_${_comp}
    REQUIRED_VARS ${_comp}_INCLUDES_DIRS
    VERSION_VAR PC_${_comp}_VERSION
    HANDLE_COMPONENTS
  )

  if(Xcb_${_comp}_FOUND AND NOT TARGET Xcb::${_comp})
    add_library(Xcb::${_comp} INTERFACE IMPORTED)
    set_target_properties(Xcb::${_comp} PROPERTIES
      INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_comp}_INCLUDES"
      INTERFACE_LINK_LIBRARIES "${PC_${_comp}_LIBRARIES}"
    )
  elseif(NOT Xcb_${_comp}_FOUND AND Xcb_FIND_REQUIRED)
      message(FATAL_ERROR "Xcb: Required component \"${component}\" is not found")
  endif()
endforeach()

cmake_policy(POP)
