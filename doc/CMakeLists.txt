find_program(SPHINX_EXECUTABLE
  NAMES sphinx-build
  DOC "Sphinx Documentation Builder")

if(NOT SPHINX_EXECUTABLE)
  message_colored(FATAL_ERROR "Failed to locate sphinx-build" 31)
endif()

# Configures conf.py in the current folder and puts it in the build folder
configure_file(conf.py conf.py @ONLY)

set(SPHINX_FLAGS "" CACHE STRING "Flags to pass to sphinx-build")
separate_arguments(sphinx_flags UNIX_COMMAND "${SPHINX_FLAGS}")

set(doc_path "${CMAKE_CURRENT_SOURCE_DIR}")

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
            -c "${CMAKE_CURRENT_BINARY_DIR}"
            -d "${CMAKE_CURRENT_BINARY_DIR}/doctrees"
            ${sphinx_flags}
            "${CMAKE_SOURCE_DIR}/doc"
            "${CMAKE_CURRENT_BINARY_DIR}/${builder}" > ${builder_log}
    COMMENT "sphinx-build ${builder}: see doc/${builder_log}")

  list(APPEND doc_targets ${doc_target})
endforeach()

# Dummy target that depends on all documentation targets
add_custom_target(doc ALL DEPENDS ${doc_targets})

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
  DESTINATION share/doc/polybar
  COMPONENT doc)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/polybar.1
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1
  COMPONENT doc)
