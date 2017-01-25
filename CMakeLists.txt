#
# Build configuration
#
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(polybar C CXX)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${PROJECT_SOURCE_DIR}/cmake
  ${PROJECT_SOURCE_DIR}/cmake/modules)

include(utils)
include(build/core)
include(build/options)
include(build/targets)
include(build/summary)

add_subdirectory(${PROJECT_SOURCE_DIR}/man)
add_subdirectory(${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/bin)
add_subdirectory(${PROJECT_SOURCE_DIR}/doc ${PROJECT_BINARY_DIR}/doc)
add_subdirectory(${PROJECT_SOURCE_DIR}/doc/zsh ${PROJECT_BINARY_DIR}/doc/zsh)

if(BUILD_TESTS)
  add_subdirectory(${PROJECT_SOURCE_DIR}/tests ${PROJECT_BINARY_DIR}/tests)
else()
  add_subdirectory(${PROJECT_SOURCE_DIR}/tests ${PROJECT_BINARY_DIR}/tests EXCLUDE_FROM_ALL)
endif()
