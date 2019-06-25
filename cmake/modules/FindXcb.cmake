# This script only supports the following components of XCB
set(XCB_known_components
  XCB
  RANDR
  COMPOSITE
  XKB
  XRM
  CURSOR)

# Deducing header from the name of the component
foreach(_comp ${XCB_known_components})
  string(TOLOWER "${_comp}" _lc_comp)
  set(XCB_${_comp}_pkg_config "xcb-${_lc_comp}")
  set(XCB_${_comp}_header "xcb/${_lc_comp}.h")
endforeach()
# Exception cases
set(XCB_XRM_header "xcb/xcb_xrm.h")
set(XCB_CURSOR_header "xcb/xcb_cursor.h")

cmake_policy(PUSH)
cmake_policy(SET CMP0057 NEW)

foreach(_comp ${Xcb_FIND_COMPONENTS})
  if (NOT ${_comp} IN_LIST XCB_known_components)
    cmake_policy(POP)
    message(FATAL_ERROR "Unknow component \"${_comp}\" of XCB")
  endif()

  find_package_impl(${XCB_${_comp}_pkg_config} "Xcb_${_comp}" "${XCB_${_comp}_header}")

  if(Xcb_${_comp}_FOUND AND NOT TARGET Xcb::${_comp})
    create_imported_target("Xcb::${_comp}" "${Xcb_${_comp}_INCLUDE_DIRS}" "${Xcb_${_comp}_LIBRARIES}")
  elseif(NOT Xcb_${_comp}_FOUND AND Xcb_FIND_REQUIRED)
    cmake_policy(POP)
    message(FATAL_ERROR "Xcb: Required component \"${_comp}\" is not found")
  endif()
endforeach()

cmake_policy(POP)
