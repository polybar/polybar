cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(testsuite CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPPUNIT_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wno-unused-parameter -Wno-unused-local-typedefs")

find_package(CppUnit REQUIRED)
add_definitions(${CPPUNIT_CFLAGS_OTHER})

file(GLOB_RECURSE source_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

make_executable(testrunner
  SOURCES
    ${source_files}
  TARGET_DEPENDS
    lemonlib_static
  CMAKE_DEPENDS
    cppunit
    xpp)

#add_custom_command(TARGET testrunner POST_BUILD COMMAND testrunner)
add_custom_target(tests testrunner
  DEPENDS testrunner
  COMMENT "Running CppUnit tests...")
