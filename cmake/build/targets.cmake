#
# Custom targets
#

# Target: uninstall {{{

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/templates/uninstall.cmake.in
  ${PROJECT_BINARY_DIR}/uninstall.cmake
  IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND}
  -P ${PROJECT_BINARY_DIR}/cuninstall.cmake)

# }}}
# Target: clang-format {{{

find_program(CLANG_FORMAT "clang-format")

if(CLANG_FORMAT)
  file(GLOB_RECURSE HEADERS ${PROJECT_SOURCE_DIR}/include/*.hpp)
  file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
  add_custom_target(clang-format COMMAND ${CLANG_FORMAT}
    -i -style=file ${HEADERS} ${SOURCES})
endif()

find_program(CLANG_TIDY "clang-tidy")

# }}}
# Target: clang-tidy {{{

if(CLANG_TIDY)
  add_custom_target(clang-tidy
    COMMAND ${CLANG_TIDY} -p
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/src/main.cpp)
endif()

# }}}
