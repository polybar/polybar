#
# Based on https://github.com/modern-cpp-examples/match3/blob/master/test/CMakeLists.txt
#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -include common/test.hpp")

include_directories(
  ${APP_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})
link_libraries(${APP_LIBRARIES})

function(unit_test file)
  string(REPLACE "/" "_" testname ${file})
  add_executable(unit_test.${testname} ${CMAKE_CURRENT_LIST_DIR}/unit_tests/${file}.cpp ${SOURCE_DEPS})
  add_test(unit_test.${testname} unit_test.${testname})
endfunction()

unit_test("utils/color")
unit_test("utils/math")
unit_test("utils/memory")
unit_test("utils/string")
unit_test("components/command_line")
unit_test("components/di")
unit_test("x11/color")

# XXX: Requires mocked xcb connection
#unit_test("x11/connection")
#unit_test("x11/winspec")
