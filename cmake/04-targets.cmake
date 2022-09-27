#
# Custom targets
#

# Target: uninstall {{{

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/templates/uninstall.cmake.in
  ${PROJECT_BINARY_DIR}/cmake/uninstall.cmake
  ESCAPE_QUOTES @ONLY)

add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/cmake/uninstall.cmake)

# }}}

# folders where the clang tools should operate
set(CLANG_SEARCH_PATHS ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests)

# Runs clang-format on all source files
add_custom_target(
  clangformat
  COMMAND ${PROJECT_SOURCE_DIR}/common/file-runner.py
  --dirs ${CLANG_SEARCH_PATHS}
  -- clang-format -style=file -i --verbose
  )

# Dry-runs clang-format on all source files
# Useful for CI since it will exit with an error code
add_custom_target(
  clangformat-dryrun
  COMMAND ${PROJECT_SOURCE_DIR}/common/file-runner.py
  --dirs ${CLANG_SEARCH_PATHS}
  -- clang-format -style=file --dry-run -Werror --verbose
  )

# Target: codecheck (clang-tidy) {{{

add_custom_target(codecheck)
add_custom_command(TARGET codecheck
  COMMAND ${PROJECT_SOURCE_DIR}/common/clang-tidy.sh
  ${PROJECT_BINARY_DIR} ${CLANG_SEARCH_PATHS})

# }}}
# Target: codecheck-fix (clang-tidy + clang-format) {{{

add_custom_target(codecheck-fix)
add_custom_command(TARGET codecheck-fix
  COMMAND ${PROJECT_SOURCE_DIR}/common/clang-tidy.sh
  ${PROJECT_BINARY_DIR} -fix ${CLANG_SEARCH_PATHS})

# }}}

# Target: memcheck (valgrind) {{{

add_custom_target(memcheck)
add_custom_command(TARGET memcheck
  COMMAND valgrind
  --leak-check=summary
  --suppressions=${PROJECT_SOURCE_DIR}/.valgrind-suppressions
  ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}
  example --config=${PROJECT_SOURCE_DIR}/doc/config)

add_custom_target(memcheck-full)
add_custom_command(TARGET memcheck-full
  COMMAND valgrind
  --leak-check=full
  --track-origins=yes
  --track-fds=yes
  --suppressions=${PROJECT_SOURCE_DIR}/.valgrind-suppressions
  ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}
  example --config=${PROJECT_SOURCE_DIR}/doc/config)

# }}}
