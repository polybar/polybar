# Project setup

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(cctz CXX)

add_library(${PROJECT_NAME} STATIC
  ${PROJECT_SOURCE_DIR}/include/cctz/civil_time_detail.h
  ${PROJECT_SOURCE_DIR}/include/cctz/civil_time.h
  ${PROJECT_SOURCE_DIR}/include/cctz/time_zone.h
  ${PROJECT_SOURCE_DIR}/include/cctz/zone_info_source.h
  ${PROJECT_SOURCE_DIR}/src/civil_time_detail.cc
  ${PROJECT_SOURCE_DIR}/src/time_tool.cc
  ${PROJECT_SOURCE_DIR}/src/time_zone_fixed.cc
  ${PROJECT_SOURCE_DIR}/src/time_zone_fixed.h
  ${PROJECT_SOURCE_DIR}/src/time_zone_format.cc
  ${PROJECT_SOURCE_DIR}/src/time_zone_if.cc
  ${PROJECT_SOURCE_DIR}/src/time_zone_if.h
  ${PROJECT_SOURCE_DIR}/src/time_zone_impl.cc
  ${PROJECT_SOURCE_DIR}/src/time_zone_impl.h
  ${PROJECT_SOURCE_DIR}/src/time_zone_info.cc
  ${PROJECT_SOURCE_DIR}/src/time_zone_info.h
  ${PROJECT_SOURCE_DIR}/src/time_zone_libc.cc
  ${PROJECT_SOURCE_DIR}/src/time_zone_libc.h
  ${PROJECT_SOURCE_DIR}/src/time_zone_lookup.cc
  ${PROJECT_SOURCE_DIR}/src/time_zone_posix.cc
  ${PROJECT_SOURCE_DIR}/src/time_zone_posix.h
  ${PROJECT_SOURCE_DIR}/src/tzfile.h
)

set (CXX_FLAGS -std=c++11 -Wno-error=unused-parameter -Wno-error=missing-field-initializers)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CXX_FLAGS ${CXX_FLAGS} "-Wno-nested-anon-types")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_options(${PROJECT_NAME} PRIVATE ${CXX_FLAGS})

# Export lists to the parent scope if there are any {{{

get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
  set(CCTZ_LIBRARIES ${PROJECT_NAME} PARENT_SCOPE)
  set(CCTZ_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)
endif()

# }}}
