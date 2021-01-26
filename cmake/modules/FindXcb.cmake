# Loads multiple XCB components
# Version checks will be made against all requested components
#
# For each component ${comp} it does the following:
#
# Defines an imported target `Xcb::${comp}` if xcb-${comp} is found
#
# Defines the following Variables (see find_package_impl for more info):
# Xcb_${comp}_FOUND
# Xcb_${comp}_INCLUDE_DIR
# Xcb_${comp}_INCLUDE_DIRS
# Xcb_${comp}_LIBRARY
# Xcb_${comp}_LIBRARIES
# Xcb_${comp}_VERSION

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

foreach(_comp ${Xcb_FIND_COMPONENTS})
  if (NOT ${_comp} IN_LIST XCB_known_components)
    message(FATAL_ERROR "Unknow component \"${_comp}\" of XCB")
  endif()

  # Forward the different find options set for FindXcb to the individual
  # components. This is required because find_package_handle_standard_args in
  # find_package_impl uses these variables for version checks and other things.
  set(Xcb_${_comp}_FIND_VERSION ${Xcb_FIND_VERSION})
  set(Xcb_${_comp}_FIND_QUIETLY ${Xcb_FIND_QUIETLY})
  set(Xcb_${_comp}_FIND_REQUIRED ${Xcb_FIND_REQUIRED})

  # Bypass developer warning that the first argument to
  # find_package_handle_standard_args (Xcb_...) does not match the name of the
  # calling package (Xcb)
  # https://cmake.org/cmake/help/v3.17/module/FindPackageHandleStandardArgs.html
  set(FPHSA_NAME_MISMATCHED TRUE)
  find_package_impl(${XCB_${_comp}_pkg_config} "Xcb_${_comp}" "${XCB_${_comp}_header}")

  if(Xcb_${_comp}_FOUND AND NOT TARGET Xcb::${_comp})
    create_imported_target("Xcb::${_comp}" "${Xcb_${_comp}_INCLUDE_DIRS}" "${Xcb_${_comp}_LIBRARIES}")
  elseif(NOT Xcb_${_comp}_FOUND AND Xcb_FIND_REQUIRED)
    message(FATAL_ERROR "Xcb: Required component \"${_comp}\" was not found")
  endif()
endforeach()
