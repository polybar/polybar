#
# Collection of cmake utility functions
#

# message_colored {{{

function(message_colored message_level text color)
  string(ASCII 27 esc)
  message(${message_level} "${esc}[${color}m${text}${esc}[0m")
endfunction()

# }}}
# colored_option {{{

function(colored_option text flag)
  if(ARGC GREATER 2 AND NOT "${${ARGV2}}" STREQUAL "")
    set(text "${text} (${${ARGV2}})")
  endif()

  if(${flag})
    message_colored(STATUS "[X]${text}" "32;1")
  else()
    message_colored(STATUS "[ ]${text}" "37;2")
  endif()
endfunction()

# }}}

# queryfont {{{

function(queryfont output_variable fontname)
  set(multi_value_args FIELDS)
  cmake_parse_arguments(ARG "" "" "${multi_value_args}" ${ARGN})

  find_program(BIN_FCLIST fc-list)
  if(NOT BIN_FCLIST)
    message_colored(WARNING "Failed to locate `fc-list`" "33;1")
    return()
  endif()

  string(REPLACE ";" " " FIELDS "${ARG_FIELDS}")
  if(NOT FIELDS)
    set(FIELDS family)
  endif()

  execute_process(
    COMMAND sh -c "${BIN_FCLIST} : ${FIELDS}"
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  STRING(REGEX REPLACE ";" "\\\\;" output "${output}")
  STRING(REGEX REPLACE "\n" ";" output "${output}")
  STRING(TOLOWER "${output}" output)

  foreach(match LISTS ${output})
    if(${match} MATCHES ".*${fontname}.*$")
      list(APPEND matches ${match})
    endif()
  endforeach()

  if(matches)
    list(GET matches 0 fst_match)
    set(${output_variable} "${fst_match}" PARENT_SCOPE)
    message(STATUS "Found font: ${fst_match}")
  else()
    message_colored(STATUS "Font not found: ${fontname}" "33;1")
  endif()
endfunction()

# }}}
# find_package_impl {{{

# Uses PkgConfig to search for pkg_config_name
#
# Defines the following variables:
# ${find_pkg_name}_FOUND - True if the package has been found
# ${find_pkg_name}_INCLUDE_DIR - <...>_INCLUDE_DIRS exported by pkg_check_modules
# ${find_pkg_name}_INCLUDE_DIRS - Same as ${find_pkg_name}_INCLUDE_DIR
# ${find_pkg_name}_LIBRARY - <...>_LIBRARIES exported by pkg_check_modules
# ${find_pkg_name}_LIBRARIES - Same as ${find_pkg_name}_LIBRARY
# ${find_pkg_name}_VERSION - <...>_VERSION exported by pkg_check_modules
#
macro(find_package_impl pkg_config_name find_pkg_name header_to_find)
  find_package(PkgConfig REQUIRED)
  include(FindPackageHandleStandardArgs)

  pkg_check_modules(PC_${find_pkg_name} REQUIRED ${pkg_config_name})

  if (NOT ${header_to_find} STREQUAL "")
    find_path(PC_${find_pkg_name}_INCLUDE_DIRS_
      NAMES "${header_to_find}"
      HINTS "${PC_${find_pkg_name}_INCLUDE_DIRS}"
    )
    set(PC_${find_pkg_name}_INCLUDE_DIRS ${PC_${find_pkg_name}_INCLUDE_DIRS_})
  endif()

  set(${find_pkg_name}_INCLUDE_DIR ${PC_${find_pkg_name}_INCLUDE_DIRS})
  set(${find_pkg_name}_INCLUDE_DIRS ${${find_pkg_name}_INCLUDE_DIR})
  set(${find_pkg_name}_LIBRARY ${PC_${find_pkg_name}_LIBRARIES})
  set(${find_pkg_name}_VERSION ${PC_${find_pkg_name}_VERSION})
  set(${find_pkg_name}_LIBRARIES ${${find_pkg_name}_LIBRARY})

  find_package_handle_standard_args(${find_pkg_name}
    REQUIRED_VARS
    ${find_pkg_name}_INCLUDE_DIRS
    ${find_pkg_name}_LIBRARIES
    VERSION_VAR
    ${find_pkg_name}_VERSION
  )

  mark_as_advanced(${find_pkg_name}_INCLUDE_DIR ${find_pkg_name}_LIBRARY)
endmacro()

# }}}
# create_imported_target {{{
function(create_imported_target library_name includes libraries)
  add_library(${library_name} INTERFACE IMPORTED)
  set_target_properties(${library_name} PROPERTIES
    INTERFACE_LINK_LIBRARIES "${libraries}"
    INTERFACE_INCLUDE_DIRECTORIES "${includes}"
  )
endfunction()
# }}}
# checklib {{{

function(checklib flag type pkg)
  if(NOT DEFINED ${flag})
    if(${type} STREQUAL "cmake")
      find_package(${pkg} QUIET)
      set(${flag} ${${pkg}_FOUND} CACHE BOOL "")
    elseif(${type} STREQUAL "pkg-config")
      find_package(PkgConfig REQUIRED)
      pkg_check_modules(PKG_${flag} QUIET ${pkg})
      set(${flag} ${PKG_${flag}_FOUND} CACHE BOOL "")
    elseif(${type} STREQUAL "binary")
      find_program(BIN_${flag} ${pkg})
      set(${flag} ${BIN_${flag}} CACHE BOOL "")
    else()
      message(FATAL_ERROR "Invalid lookup type '${type}'")
    endif()
    mark_as_advanced(${flag})
  endif()
endfunction()

function(get_include_dirs output)
  get_filename_component(generated_sources_dir  ${CMAKE_BINARY_DIR}/generated-sources   ABSOLUTE)
  get_filename_component(include_dir            ${CMAKE_SOURCE_DIR}/include             ABSOLUTE)

  set(${output} ${include_dir} ${generated_sources_dir} PARENT_SCOPE)
endfunction()

function(get_sources_dirs output)
  get_filename_component(src_dir                ${CMAKE_SOURCE_DIR}/src                 ABSOLUTE)

  set(${output} ${src_dir} PARENT_SCOPE)
endfunction()

# }}}
