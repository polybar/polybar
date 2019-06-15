
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LibPulse QUIET libpulse)

include(FindPackageHandleStandardArgs)

find_path(LibPulse_INCLUDES
  NAMES pulse/version.h
  HINTS ${PC_LibPulse_INCLUDEDIR} ${PC_LibPulse_INCLUDE_DIRS}
)

find_package_handle_standard_args(LibPulse
  REQUIRED_VARS LibPulse_INCLUDES
  VERSION_VAR PC_LibPulse_VERSION
)

if(LibPulse_FOUND AND NOT TARGET LibPulse::LibPulse)
  add_library(LibPulse::LibPulse INTERFACE IMPORTED)
  set_target_properties(LibPulse::LibPulse PROPERTIES
    INTERFACE_LINK_LIBRARIES "${PC_LibPulse_LIBRARIES}")

  target_include_directories(LibPulse::LibPulse SYSTEM INTERFACE ${LibPulse_INCLUDES})
endif()
