# This module defines an imported target `LibPulse::LibPulse` if libpulse is found
#
# Defines the following Variables (see find_package_impl for more info):
# LibPulse_FOUND
# LibPulse_INCLUDE_DIR
# LibPulse_INCLUDE_DIRS
# LibPulse_LIBRARY
# LibPulse_LIBRARIES
# LibPulse_VERSION
find_package_impl("libpulse" "LibPulse" "pulse/version.h")

if(LibPulse_FOUND AND NOT TARGET LibPulse::LibPulse)
  create_imported_target("LibPulse::LibPulse" "${LibPulse_INCLUDE_DIR}" "${LibPulse_LIBRARY}")
endif()
