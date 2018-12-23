#
# Build configuration
#
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# Enable ccache by default and as early as possible because project() performs
# checks on the compiler
option(ENABLE_CCACHE "Enable ccache support" ON)
if(ENABLE_CCACHE)
  message(STATUS "Trying to enable ccache")
  find_program(BIN_CCACHE ccache)

  string(ASCII 27 esc)
  if(NOT BIN_CCACHE)
    message(STATUS "${esc}[33mCouldn't locate ccache, disabling ccache...${esc}[0m")
  else()
    # Enable only if the binary is found
    message(STATUS "${esc}[32mUsing compiler cache ${BIN_CCACHE}${esc}[0m")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${BIN_CCACHE})
  endif()
endif()

project(polybar CXX)

# Extract version information from version.txt. The first line that looks like
# a version string is used, so the file supports comments
file(STRINGS version.txt version_txt REGEX "^[0-9]+\\.[0-9]+\\.[0-9]+.*$" LIMIT_COUNT 1)

# If we are in a git repo we can get the version information from git describe
execute_process(COMMAND git describe --tags --dirty=-dev
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_result
  OUTPUT_VARIABLE git_describe
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

if(git_result EQUAL "0")
  set(APP_VERSION "${git_describe}")
else()
  message(STATUS "Could not detect version with git, falling back to built-in version information.")
  set(APP_VERSION "${version_txt}")
endif()

STRING(REGEX REPLACE "[^a-zA-Z0-9_]" "_" APP_VERSION_NAMESPACE "v${APP_VERSION}")

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

if(BUILD_DOC)
  add_subdirectory(doc)
endif()
add_subdirectory(doc/bash)
add_subdirectory(doc/zsh)
add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(src bin)

# We need to enable testing in the root folder so that 'ctest' and 'make test'
# can be run in the build directory
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
