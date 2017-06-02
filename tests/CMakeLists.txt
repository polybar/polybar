#
# Based on https://github.com/modern-cpp-examples/match3/blob/master/test/CMakeLists.txt
#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -include common/test.hpp")

link_libraries(${libs})
include_directories(${dirs})
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_LIST_DIR})

function(unit_test file)
  string(REPLACE "/" "_" testname ${file})
  add_executable(unit_test.${testname} unit_tests/${file}.cpp)
  add_test(unit_test.${testname} unit_test.${testname})
endfunction()

unit_test(utils/color)
unit_test(utils/math)
unit_test(utils/memory)
unit_test(utils/string)
unit_test(utils/file)
unit_test(components/command_line)

# XXX: Requires mocked xcb connection
#unit_test("x11/connection")
#unit_test("x11/winspec")
