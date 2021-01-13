#
# Generate settings.hpp
#

list(APPEND XPP_EXTENSION_LIST xpp::randr::extension)
list(APPEND XPP_EXTENSION_LIST xpp::composite::extension)
if(WITH_XKB)
  list(APPEND XPP_EXTENSION_LIST xpp::xkb::extension)
endif()
string(REPLACE ";" ", " XPP_EXTENSION_LIST "${XPP_EXTENSION_LIST}")

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/settings.hpp.cmake
  ${CMAKE_BINARY_DIR}/generated-sources/settings.hpp
  ESCAPE_QUOTES)
