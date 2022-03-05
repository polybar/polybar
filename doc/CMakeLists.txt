set(SPHINX_BUILD "sphinx-build" CACHE STRING "Name/Path of the sphinx-build executable to use.")
set(SPHINX_FLAGS "" CACHE STRING "Flags to pass to sphinx-build")

find_program(BIN_SPHINX "${SPHINX_BUILD}")

if(NOT BIN_SPHINX)
  message(FATAL_ERROR "sphinx-build executable '${SPHINX_BUILD}' not found.")
endif()

separate_arguments(sphinx_flags UNIX_COMMAND "${SPHINX_FLAGS}")

set(doc_path "${CMAKE_CURRENT_SOURCE_DIR}")

# Configures conf.py in the current folder and puts it in the build folder
configure_file(conf.py conf.py @ONLY)

# We want to run `sphinx-build` with the following builders
if (BUILD_DOC_HTML)
  list(APPEND doc_builders "html")
endif()

if (BUILD_DOC_MAN)
  list(APPEND doc_builders "man")
endif()

# Name of all documentation targets
set(doc_targets "")

foreach(builder ${doc_builders})
  set(doc_target "doc_${builder}")
  set(builder_log "builder-${builder}.log")
  add_custom_target(${doc_target}
    COMMAND ${BIN_SPHINX}
            -b ${builder}
            # conf.py dir
            -c "${CMAKE_CURRENT_BINARY_DIR}"
            -d "${CMAKE_CURRENT_BINARY_DIR}/doctrees"
            -n
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

if (BUILD_DOC_HTML)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
  COMPONENT doc
  PATTERN ".buildinfo" EXCLUDE)
endif()

install(FILES ${CMAKE_SOURCE_DIR}/CHANGELOG.md
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
  COMPONENT doc)

install(FILES ${CMAKE_CURRENT_LIST_DIR}/config.ini
  DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
  COMPONENT doc)

if (BUILD_DOC_MAN)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/polybar.1
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
  COMPONENT doc)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/polybar-msg.1
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
  COMPONENT doc)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/polybar.5
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man5
  COMPONENT doc)
endif()

