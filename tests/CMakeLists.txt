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

# Compile all unit tests with 'make all_unit_tests'
add_custom_target(all_unit_tests
    COMMENT "Building all unit test")

function(add_unit_test source_file)
  string(REPLACE "/" "_" testname ${source_file})
  set(name "unit_test.${testname}")

  add_executable(${name} unit_tests/${source_file}.cpp)
  get_include_dirs(includes_dir)
  target_include_directories(${name} PRIVATE ${includes_dir} ${CMAKE_CURRENT_LIST_DIR})

  # Link against gmock (this automatically links against gtest)
  target_link_libraries(${name} poly gmock_main)
  add_test(NAME ${name} COMMAND ${name})

  add_dependencies(all_unit_tests ${name})
endfunction()

add_unit_test(utils/actions)
add_unit_test(utils/action_router)
add_unit_test(utils/color)
add_unit_test(utils/command)
add_unit_test(utils/env)
add_unit_test(utils/math)
add_unit_test(utils/scope)
add_unit_test(utils/string)
add_unit_test(utils/file)
add_unit_test(utils/process)
add_unit_test(utils/units)
add_unit_test(components/builder)
add_unit_test(components/command_line)
add_unit_test(components/config_parser)
add_unit_test(drawtypes/label)
add_unit_test(drawtypes/ramp)
add_unit_test(drawtypes/iconset)
add_unit_test(drawtypes/layouticonset)
add_unit_test(ipc/decoder)
add_unit_test(ipc/encoder)
add_unit_test(ipc/util)
add_unit_test(tags/parser)
add_unit_test(tags/dispatch)
add_unit_test(tags/action_context)

# Run make check to build and run all unit tests
add_custom_target(check
  COMMAND GTEST_COLOR=1 ctest --output-on-failure
  DEPENDS all_unit_tests
  )
