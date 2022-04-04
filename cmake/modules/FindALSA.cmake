# This module defines an imported target `ALSA::ALSA` if alsa is found
#
# Defines the following Variables (see find_package_impl for more info):
# ALSA_FOUND
# ALSA_INCLUDE_DIR
# ALSA_INCLUDE_DIRS
# ALSA_LIBRARY
# ALSA_LIBRARIES
# ALSA_VERSION
find_package_impl("alsa" "ALSA" "alsa/asoundlib.h")

if(ALSA_FOUND AND NOT TARGET ALSA::ALSA)
  create_imported_target("ALSA::ALSA" "${ALSA_INCLUDE_DIR}" "${ALSA_LIBRARY}")
endif()
