#
# Build configuration
#
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(polybar C CXX)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${PROJECT_SOURCE_DIR}/cmake
  ${PROJECT_SOURCE_DIR}/cmake/common
  ${PROJECT_SOURCE_DIR}/cmake/modules)

include(utils)
include(01-core)
include(02-opts)
include(03-libs)
include(04-targets)
include(05-summary)

add_subdirectory(doc)
add_subdirectory(doc/bash)
add_subdirectory(doc/zsh)
add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(man)
add_subdirectory(src bin)

if(BUILD_TESTS)
  add_subdirectory(tests)
else()
  add_subdirectory(tests EXCLUDE_FROM_ALL)
endif()
