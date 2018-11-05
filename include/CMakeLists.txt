#
# Generate settings.hpp
#

list(APPEND dirs ${CMAKE_CURRENT_LIST_DIR})

if(WITH_XRANDR)
  list(APPEND XPP_EXTENSION_LIST xpp::randr::extension)
endif()
if(WITH_XRENDER)
  list(APPEND XPP_EXTENSION_LIST xpp::render::extension)
endif()
if(WITH_XDAMAGE)
  list(APPEND XPP_EXTENSION_LIST xpp::damage::extension)
endif()
if(WITH_XSYNC)
  list(APPEND XPP_EXTENSION_LIST xpp::sync::extension)
endif()
if(WITH_XCOMPOSITE)
  list(APPEND XPP_EXTENSION_LIST xpp::composite::extension)
endif()
if(WITH_XKB)
  list(APPEND XPP_EXTENSION_LIST xpp::xkb::extension)
endif()
string(REPLACE ";" ", " XPP_EXTENSION_LIST "${XPP_EXTENSION_LIST}")

execute_process(COMMAND git describe --tags --dirty=-git
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE APP_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

if(APP_VERSION)
  STRING(REGEX REPLACE "[^a-zA-Z0-9_]" "_" APP_VERSION_NAMESPACE "v${APP_VERSION}")
endif()

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/settings.hpp.cmake
  ${CMAKE_CURRENT_LIST_DIR}/settings.hpp
  ESCAPE_QUOTES @ONLY)

set(APP_VERSION ${APP_VERSION} PARENT_SCOPE)
set(dirs ${dirs} PARENT_SCOPE)
