#
# Collection of cmake utility functions
#

# message_colored : Outputs a colorized message {{{

function(message_colored message_level text color)
  string(ASCII 27 esc)
  message(${message_level} "${esc}[${color}m${text}${esc}[0m")
endfunction()

# }}}
# require_binary : Locates binary by name and exports its path to BINPATH_${name} {{{

function(require_binary binary_name)
  find_program(BINPATH_${binary_name} ${binary_name})
  if(NOT BINPATH_${binary_name})
    message_colored(FATAL_ERROR "Failed to locate ${binary_name} binary" 31)
  endif()
endfunction()

# }}}
# make_executable : Builds an executable target {{{

function(make_executable target_name)
  set(zero_value_args)
  set(one_value_args PACKAGE)
  set(multi_value_args SOURCES INCLUDE_DIRS PKG_DEPENDS CMAKE_DEPENDS TARGET_DEPENDS RAW_DEPENDS)

  cmake_parse_arguments(BIN
    "${zero_value_args}" "${one_value_args}"
    "${multi_value_args}" ${ARGN})

  # add defined INCLUDE_DIRS
  include_directories(${BIN_INCLUDE_DIRS})

  # add INCLUDE_DIRS for all external dependencies
  foreach(DEP ${BIN_TARGET_DEPENDS} ${BIN_PKG_DEPENDS} ${BIN_CMAKE_DEPENDS})
    string(TOUPPER ${DEP} DEP)
    include_directories(${${DEP}_INCLUDE_DIRS})
    include_directories(${${DEP}_INCLUDEDIR})
  endforeach()

  # create target
  add_executable(${target_name} ${BIN_SOURCES})

  # set the output file basename the same for static and shared
  set_target_properties(${target_name}
                        PROPERTIES OUTPUT_NAME ${target_name})

  # link libraries from pkg-config imports
  foreach(DEP ${BIN_PKG_DEPENDS})
    string(TOUPPER ${DEP} DEP)
    target_link_libraries(${target_name} ${${DEP}_LDFLAGS})
  endforeach()

  # link libraries from cmake imports
  foreach(DEP ${BIN_CMAKE_DEPENDS})
    string(TOUPPER ${DEP} DEP)
    target_link_libraries(${target_name} ${${DEP}_LIB}
                                             ${${DEP}_LIBRARY}
                                             ${${DEP}_LIBRARIES})
  endforeach()

  # link libraries that are build as part of this project
  target_link_libraries(${target_name} ${BIN_TARGET_DEPENDS} ${BIN_RAW_DEPENDS})

  # install targets
  install(TARGETS ${target_name}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          COMPONENT runtime)
endfunction()

# }}}
# make_library : Builds a library target {{{

function(make_library target_name)
  set(zero_value_args SHARED STATIC)
  set(one_value_args PACKAGE HEADER_INSTALL_DIR)
  set(multi_value_args SOURCES HEADERS INCLUDE_DIRS PKG_DEPENDS CMAKE_DEPENDS TARGET_DEPENDS RAW_DEPENDS)

  cmake_parse_arguments(LIB
    "${zero_value_args}" "${one_value_args}"
    "${multi_value_args}" ${ARGN})

  # make the header paths absolute
  foreach(HEADER ${LIB_HEADERS})
    set(LIB_HEADERS_ABS ${LIB_HEADERS_ABS} ${PROJECT_SOURCE_DIR}/include/${HEADER})
  endforeach()

  # add defined INCLUDE_DIRS
  foreach(DIR ${LIB_INCLUDE_DIRS})
    string(TOUPPER ${DIR} DIR)
    include_directories(${DIR})
    include_directories(${${DIR}_INCLUDE_DIRS})
  endforeach()

  # add INCLUDE_DIRS for all external dependencies
  foreach(DEP ${LIB_TARGET_DEPENDS} ${LIB_PKG_DEPENDS} ${LIB_CMAKE_DEPENDS})
    string(TOUPPER ${DEP} DEP)
    include_directories(${${DEP}_INCLUDE_DIRS} ${${DEP}_INCLUDEDIRS})
  endforeach()

  if(LIB_SHARED)
    list(APPEND library_targets ${target_name}_shared)
  endif()
  if(LIB_STATIC)
    list(APPEND library_targets ${target_name}_static)
  endif()

  foreach(library_target_name ${library_targets})
    message(STATUS "${library_target_name}")
    add_library(${library_target_name} ${LIB_HEADERS_ABS} ${LIB_SOURCES})

    # link libraries from pkg-config imports
    foreach(DEP ${LIB_PKG_DEPENDS})
      string(TOUPPER ${DEP} DEP)
      target_link_libraries(${library_target_name} ${${DEP}_LDFLAGS})
    endforeach()

    # link libraries from cmake imports
    foreach(DEP ${LIB_CMAKE_DEPENDS})
      string(TOUPPER ${DEP} DEP)
      target_link_libraries(${library_target_name} ${${DEP}_LIB}
                                                   ${${DEP}_LIBRARY}
                                                   ${${DEP}_LIBRARIES})
    endforeach()

    # link libraries that are build as part of this project
    foreach(DEP ${LIB_TARGET_DEPENDS})
      string(TOUPPER ${DEP} DEP)
      if(LIB_BUILD_SHARED)
        target_link_libraries(${library_target_name} ${DEP}_shared)
      endif()
      if(LIB_BUILD_STATIC)
        target_link_libraries(${library_target_name} ${DEP}_static)
      endif()
    endforeach()

    if(${LIB_RAW_DEPENDS})
      if(LIB_BUILD_STATIC)
        target_link_libraries(${library_target_name} ${LIB_RAW_DEPENDS})
      endif()
    endif()

    # set the output file basename
    set_target_properties(${library_target_name} PROPERTIES OUTPUT_NAME ${target_name})

    # install headers
    install(FILES ${LIBRARY_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_HEADERS_ABS})

    # install targets
    install(TARGETS ${LIBRARY_TARGETS}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library)
  endforeach()
endfunction()

# }}}
# find_font : Query fontconfig fonts {{{

function(font_query output_variable fontname)
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
      list(APPEND FONT_QUERY_MATCHES ${match})
    endif()
  endforeach()

  if(FONT_QUERY_MATCHES)
    list(GET FONT_QUERY_MATCHES 0 output_variable)
    set(output_variable "${output_variable}" PARENT_SCOPE)
    message(STATUS "Found font: ${output_variable}")
  else()
    message_colored(STATUS "Font not found: ${fontname}" "33;1")
  endif()
endfunction()

# }}}
