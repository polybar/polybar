find_package_impl("libpulse" "LibPulse" "pulse/version.h")

if(LibPulse_FOUND AND NOT TARGET LibPulse::LibPulse)
  create_imported_target("LibPulse::LibPulse" "${LibPulse_INCLUDE_DIR}" "${LibPulse_LIBRARY}")
endif()
