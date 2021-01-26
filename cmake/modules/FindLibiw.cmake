# This module defines
#  LIBIW_FOUND - whether the libiw library was found
#  LIBIW_LIBRARIES - the libiw library
#  LIBIW_INCLUDE_DIR - the include path of the libiw library

find_library(LIBIW_LIBRARY iw)

if(LIBIW_LIBRARY)
  set(LIBIW_LIBRARIES ${LIBIW_LIBRARY})
endif(LIBIW_LIBRARY)

find_path(LIBIW_INCLUDE_DIR NAMES iwlib.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libiw DEFAULT_MSG LIBIW_LIBRARY LIBIW_INCLUDE_DIR)

mark_as_advanced(LIBIW_INCLUDE_DIR LIBIW_LIBRARY)

if(Libiw_FOUND AND NOT TARGET Libiw::Libiw)
  create_imported_target("Libiw::Libiw" "${LIBIW_INCLUDE_DIR}" "${LIBIW_LIBRARIES}")
endif()
