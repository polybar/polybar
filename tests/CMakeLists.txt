# Compile and link with coverage
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fprofile-arcs -ftest-coverage -Wno-missing-field-initializers")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")

include_directories(${dirs})
include_directories(${CMAKE_CURRENT_LIST_DIR})

# Download and unpack googletest at configure time {{{
configure_file(
  CMakeLists.txt.in
  ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt
  )
execute_process( COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)

if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )

if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Add googletest directly to our build. This defines
# the gtest, gtest_main, gmock and gmock_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
                 ${CMAKE_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

# }}}

# Disable errors for warnings so that we can run tests even with some warnings
string(REGEX REPLACE "-Werror[^ ]*" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "-pedantic-errors" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

# Compile all unit tests with 'make all_unit_tests'
add_custom_target(all_unit_tests
    COMMENT "Building all unit test")

function(add_unit_test source_file)
  string(REPLACE "/" "_" testname ${source_file})
  set(name "unit_test.${testname}")

  add_executable(${name} unit_tests/${source_file}.cpp)

  # Link against gmock (this automatically links against gtest)
  target_link_libraries(${name} polybar-lib gmock_main)
  add_test(NAME ${name} COMMAND ${name})

  add_dependencies(all_unit_tests ${name})
endfunction()

add_unit_test(utils/color)
add_unit_test(utils/math unit_tests)
add_unit_test(utils/memory unit_tests)
add_unit_test(utils/scope unit_tests)
add_unit_test(utils/string unit_tests)
add_unit_test(utils/file)
add_unit_test(components/command_line)
add_unit_test(components/bar)
add_unit_test(components/builder)
