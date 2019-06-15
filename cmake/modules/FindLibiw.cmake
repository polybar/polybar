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
  add_library(Libiw::Libiw INTERFACE IMPORTED)
  set_target_properties(Libiw::Libiw PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${LIBIW_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "${LIBIW_LIBRARY}")
endif()
