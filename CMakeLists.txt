#
# Build configuration
#
cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)
project(polybar CXX)

# Extract version information from version.txt. The first line that looks like
# a version string is used, so the file supports comments
file(STRINGS version.txt version_txt REGEX "^[0-9]+\\.[0-9]+\\.[0-9]+.*$" LIMIT_COUNT 1)


execute_process(COMMAND git rev-parse --show-toplevel
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_top_dir_result
  OUTPUT_VARIABLE git_top_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

# Set fallback first, override if more information (from git) is found
set(APP_VERSION "${version_txt}")

# Prevent formatting errors & resolve symlinks (REALPATH)
get_filename_component(resolved_project_source_dir ${PROJECT_SOURCE_DIR} REALPATH)
if(NOT ("${git_top_dir}" STREQUAL ""))
  get_filename_component(git_top_dir ${git_top_dir} REALPATH)
endif()

if(git_top_dir_result EQUAL "0" AND git_top_dir STREQUAL resolved_project_source_dir)
  # If we are in a git repo we can get the version information from git describe
  execute_process(COMMAND git describe --tags --dirty=-dev
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    RESULT_VARIABLE git_describe_result
    OUTPUT_VARIABLE git_describe
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

  if(git_describe_result EQUAL "0")
    set(APP_VERSION "${git_describe}")
  else()
    message(STATUS "Could not detect version with git, falling back to built-in version information.")
  endif()

else()
  message(STATUS "CMake and git directory mismatch, falling back to built-in version information.") # Assuming that if git rev-parse doesn't return 0, git describe won't either
endif()


# Set the default installation prefix to /usr
# Otherwise the default value is /usr/local which causes the default config
# file to be installed to /usr/local/etc, with /usr, cmake has special handling
# for this.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Project-default installation prefix" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake
  ${PROJECT_SOURCE_DIR}/cmake/common
  ${PROJECT_SOURCE_DIR}/cmake/modules)

include(GNUInstallDirs)
include(utils)
include(01-core)
include(02-opts)
include(04-targets)

if(BUILD_DOC)
  add_subdirectory(doc)
endif()

if (BUILD_SHELL)
  add_subdirectory(contrib/bash)
  add_subdirectory(contrib/zsh)
endif()

# Setup everything that uses a C++ compiler (polybar, polybar-msg, tests)
if(HAS_CXX_COMPILATION)
  include(cxx)
  if(BUILD_LIBPOLY)
    include(libpoly)
    add_subdirectory(lib)
  endif()
  add_subdirectory(include)
  add_subdirectory(src bin)
endif()

# We need to enable testing in the root folder so that 'ctest' and 'make test'
# can be run in the build directory
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(BUILD_CONFIG)
  install(FILES ${CMAKE_SOURCE_DIR}/doc/config.ini
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/${PROJECT_NAME}
    COMPONENT config)
endif()

include(05-summary)
