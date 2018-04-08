# Compile and link with coverage
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")

link_libraries(${libs})
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

function(unit_test file tests)
  set(multi_value_args SOURCES)

  cmake_parse_arguments("BIN" "" "" "${multi_value_args}" ${ARGN})

  # Prefix all sources needed by the tests with ../src/ so that the calls to the
  # unit_test function become cleaner
  SET(sources "")
  FOREACH(f ${BIN_SOURCES})
    LIST(APPEND sources "../src/${f}")
  ENDFOREACH(f)

  string(REPLACE "/" "_" testname ${file})
  set(name "unit_test.${testname}")
  add_executable(${name} unit_tests/${file}.cpp ${sources})

  # Link against gmock (this automatically links against gtest)
  target_link_libraries(${name} gmock_main)

  add_test(NAME ${name} COMMAND ${name})

  # Add test to list of unit tests
  list(APPEND ${tests} "${name}")
  set(${tests} ${${tests}} PARENT_SCOPE)
endfunction()

unit_test(utils/color unit_tests)
unit_test(utils/math unit_tests)
unit_test(utils/memory unit_tests)
unit_test(utils/string unit_tests
  SOURCES
  utils/string.cpp)
unit_test(utils/file unit_tests
  SOURCES
  utils/command.cpp
  utils/file.cpp
  utils/env.cpp
  utils/process.cpp
  utils/io.cpp
  utils/string.cpp
  utils/concurrency.cpp
  components/logger.cpp)
unit_test(components/command_line unit_tests
  SOURCES
  components/command_line.cpp
  utils/string.cpp)

# Compile all unit tests with 'make all_unit_tests'
add_custom_target("all_unit_tests" DEPENDS ${unit_tests})
