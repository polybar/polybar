#
# Custom targets
#

# Target: userconfig {{{

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/templates/userconfig.cmake.in
  ${PROJECT_BINARY_DIR}/userconfig.cmake
  IMMEDIATE @ONLY)

add_custom_target(userconfig COMMAND ${CMAKE_COMMAND}
  -P ${PROJECT_BINARY_DIR}/userconfig.cmake)

# }}}
# Target: uninstall {{{

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/templates/uninstall.cmake.in
  ${PROJECT_BINARY_DIR}/cmake/uninstall.cmake
  IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND}
  -P ${PROJECT_BINARY_DIR}/cmake/uninstall.cmake)

# }}}
# Target: codeformat (clang-format) {{{

add_custom_target(codeformat)
add_custom_command(TARGET codeformat COMMAND
  ${PROJECT_SOURCE_DIR}/common/clang-format.sh ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include)

# }}}
# Target: codecheck (clang-tidy) {{{

add_custom_target(codecheck)
add_custom_command(TARGET codecheck COMMAND
  ${PROJECT_SOURCE_DIR}/common/clang-tidy.sh ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/src)

# }}}
# Target: codecheck-fix (clang-tidy + clang-format) {{{

add_custom_target(codecheck-fix)
add_custom_command(TARGET codecheck-fix COMMAND
  ${PROJECT_SOURCE_DIR}/common/clang-tidy.sh ${PROJECT_BINARY_DIR} -fix ${PROJECT_SOURCE_DIR}/src)

# }}}

# Target: memcheck (valgrind) {{{

add_custom_target(memcheck)
add_custom_command(TARGET memcheck COMMAND valgrind
  --leak-check=summary
  --suppressions=${PROJECT_SOURCE_DIR}/.valgrind-suppressions
  ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME} example --config=${PROJECT_SOURCE_DIR}/examples/config)

add_custom_target(memcheck-full)
add_custom_command(TARGET memcheck-full COMMAND valgrind
  --leak-check=full
  --track-origins=yes
  --track-fds=yes
  --suppressions=${PROJECT_SOURCE_DIR}/.valgrind-suppressions
  ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME} example --config=${PROJECT_SOURCE_DIR}/examples/config)

# }}}
