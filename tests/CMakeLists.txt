#
# Based on https://github.com/modern-cpp-examples/match3/blob/master/test/CMakeLists.txt
#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -include common/test.hpp")

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${APP_INCLUDE_DIRS})

function(unit_test file)
  string(REPLACE "/" "_" testname ${file})
  add_executable(unit_test.${testname} ${CMAKE_CURRENT_LIST_DIR}/unit_tests/${file}.cpp)
  add_test(unit_test.${testname} unit_test.${testname})
endfunction()

unit_test("utils/math")
unit_test("utils/memory")
unit_test("utils/string")
unit_test("components/command_line")
unit_test("components/di")
#unit_test("components/logger")
unit_test("components/x11/color")
#unit_test("components/x11/connection")
#unit_test("components/x11/window")
