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
