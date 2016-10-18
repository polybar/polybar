#
# Additional targets to perform clang-format/clang-tidy
#

file(GLOB_RECURSE header_files ${PROJECT_SOURCE_DIR}/include/*.hpp)
file(GLOB_RECURSE source_files ${PROJECT_SOURCE_DIR}/src/*.cpp)

# Add clang-format target if executable is found
# --------------------------------------------------
find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
  add_custom_target(clang-format COMMAND ${CLANG_FORMAT}
    -i -style=file ${header_files} ${source_files})
endif()

# Add clang-tidy target if executable is found
# --------------------------------------------------
find_program(CLANG_TIDY "clang-tidy")
if(CLANG_TIDY)
  add_custom_target(clang-tidy COMMAND ${CLANG_TIDY}
    -p ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/src/main.cpp)
endif()
