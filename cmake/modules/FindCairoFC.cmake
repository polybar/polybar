# This module defines an imported target `Cairo::CairoFC` if cairo-fc is found
#
# Defines the following Variables (see find_package_impl for more info):
# CairoFC_FOUND
# CairoFC_INCLUDE_DIR
# CairoFC_INCLUDE_DIRS
# CairoFC_LIBRARY
# CairoFC_LIBRARIES
# CairoFC_VERSION
find_package_impl("cairo-fc" "CairoFC" "")

if(CairoFC_FOUND AND NOT TARGET Cairo::CairoFC)
  create_imported_target("Cairo::CairoFC" "${CairoFC_INCLUDE_DIR}" "${CairoFC_LIBRARY}")
endif()
