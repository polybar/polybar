cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# Only used if documentation is built on its own
project(polybar-doc NONE)

find_program(SPHINX_EXECUTABLE
  NAMES sphinx-build
  DOC "Sphinx Documentation Builder")

if(NOT SPHINX_EXECUTABLE)
  message_colored(FATAL_ERROR "Failed to locate sphinx-build" 31)
endif()

set(SPHINX_FLAGS "" CACHE STRING "Flags to pass to sphinx-build")
separate_arguments(sphinx_flags UNIX_COMMAND "${SPHINX_FLAGS}")

set(doc_path "${CMAKE_CURRENT_SOURCE_DIR}")

# Configures conf.py in the current folder and puts it in the build folder
configure_file(conf.py conf.py @ONLY)

# We want to run `sphinx-build` with the following builders
set(doc_builders "html" "man")

# Name of all documentation targets
set(doc_targets "")

foreach(builder ${doc_builders})
  set(doc_target "doc_${builder}")
  set(builder_log "builder-${builder}.log")
  add_custom_target(${doc_target}
    COMMAND ${SPHINX_EXECUTABLE}
            -b ${builder}
            # conf.py dir
            -c "${CMAKE_CURRENT_BINARY_DIR}"
            -d "${CMAKE_CURRENT_BINARY_DIR}/doctrees"
            ${sphinx_flags}
            # Documentation source file dir
            "${CMAKE_CURRENT_SOURCE_DIR}"
            # Output dir
            "${CMAKE_CURRENT_BINARY_DIR}/${builder}" > ${builder_log}
    COMMENT "sphinx-build ${builder}: see doc/${builder_log}")

  list(APPEND doc_targets ${doc_target})
endforeach()

# Dummy target that depends on all documentation targets
add_custom_target(doc ALL DEPENDS ${doc_targets})

# This is needed for the case where only the doc target is built
# CMAKE_INSTALL_DOCDIR uses PROJECT_NAME which is now polybar-doc, to be
# consistent with a regular install we temporarily override it with "polybar"
# before including GNUInstallDirs
# Also since no language is set and GNUInstallDirs cannot set
# CMAKE_INSTALL_LIBDIR, so we set it to a dummy value to suppress a warning
if(${CMAKE_PROJECT_NAME} STREQUAL "polybar-doc")
  set(PROJECT_NAME "polybar")
  set(CMAKE_INSTALL_LIBDIR "")
  include(GNUInstallDirs)
  set(PROJECT_NAME "polybar-doc")
endif()

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
  COMPONENT doc)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/polybar.1
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
  COMPONENT doc)
