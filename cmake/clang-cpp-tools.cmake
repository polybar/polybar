#
# Additional targets to perform clang-format/clang-tidy
#

file(GLOB_RECURSE SOURCE_FILES *.[chi]pp)

# Add clang-format target if executable is found
# --------------------------------------------------
find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
  add_custom_target(clang-format COMMAND
    ${CLANG_FORMAT} -i -style=file ${SOURCE_FILES})
endif()

# Add clang-tidy target if executable is found
# --------------------------------------------------
find_program(CLANG_TIDY "clang-tidy")
if(CLANG_TIDY)
  add_custom_target(clang-tidy COMMAND ${CLANG_TIDY}
    ${SOURCE_FILES} -config='' -- -std=c++11
    ${INCLUDE_DIRECTORIES})
endif()
