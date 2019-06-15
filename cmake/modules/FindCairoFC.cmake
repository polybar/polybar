find_package(PkgConfig REQUIRED)
include(CMakeFindDependencyMacro)

pkg_check_modules(PC_CairoFC QUIET cairo-fc)

set(CairoFC_INCLUDE_DIR ${PC_CairoFC_INCLUDE_DIRS})
set(CairoFC_LIBRARY ${PC_CairoFC_LIBRARIES})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CairoFC_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(CairoFC
  REQUIRED_VARS
  CairoFC_INCLUDE_DIR
  CairoFC_LIBRARY
  VERSION_VAR
  PC_CairoFC_VERSION)
message(STATUS "${CairoFC}")

mark_as_advanced(CairoFC_INCLUDE_DIR CairoFC_LIBRARY)

set(CairoFC_LIBRARIES ${CairoFC_LIBRARY})
set(CairoFC_INCLUDE_DIRS ${CairoFC_INCLUDE_DIR})

if(CairoFC_FOUND AND NOT TARGET Cairo::CairoFC)
  add_library(Cairo::CairoFC INTERFACE IMPORTED)
  set_target_properties(Cairo::CairoFC PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CairoFC_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${CairoFC_LIBRARIES}"
  )
endif()
