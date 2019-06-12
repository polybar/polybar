#
# Generate settings.hpp
#

if(WITH_XRANDR)
  list(APPEND XPP_EXTENSION_LIST xpp::randr::extension)
endif()
if(WITH_XCOMPOSITE)
  list(APPEND XPP_EXTENSION_LIST xpp::composite::extension)
endif()
if(WITH_XKB)
  list(APPEND XPP_EXTENSION_LIST xpp::xkb::extension)
endif()
string(REPLACE ";" ", " XPP_EXTENSION_LIST "${XPP_EXTENSION_LIST}")

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/settings.hpp.cmake
  ${CMAKE_BINARY_DIR}/generated-sources/settings.hpp
  ESCAPE_QUOTES)

set(APP_VERSION ${APP_VERSION} PARENT_SCOPE)
