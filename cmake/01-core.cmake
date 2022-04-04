#
# Core setup
#
option(DISABLE_ALL "Set this to ON disable all targets. Individual targets can be enabled explicitly." OFF)

# If all targets are disabled, we set the default value for options that are on
# by default to OFF
if (DISABLE_ALL)
  set(DEFAULT_ON OFF)
else()
  set(DEFAULT_ON ON)
endif()

option(BUILD_POLYBAR "Build the main polybar executable" ${DEFAULT_ON})
option(BUILD_POLYBAR_MSG "Build polybar-msg" ${DEFAULT_ON})
option(BUILD_TESTS "Build testsuite" OFF)
option(BUILD_DOC "Build documentation" ${DEFAULT_ON})
option(BUILD_CONFIG "Generate default configuration" ${DEFAULT_ON})
option(BUILD_SHELL "Generate shell completion files" ${DEFAULT_ON})

include(CMakeDependentOption)
CMAKE_DEPENDENT_OPTION(BUILD_DOC_HTML "Build HTML documentation" ON "BUILD_DOC" OFF)
CMAKE_DEPENDENT_OPTION(BUILD_DOC_MAN "Build manpages" ON "BUILD_DOC" OFF)

if (BUILD_POLYBAR OR BUILD_TESTS OR BUILD_POLYBAR_MSG)
  set(BUILD_LIBPOLY ON)
else()
  set(BUILD_LIBPOLY OFF)
endif()

if (BUILD_POLYBAR OR BUILD_POLYBAR_MSG OR BUILD_TESTS)
  set(HAS_CXX_COMPILATION ON)
else()
  set(HAS_CXX_COMPILATION OFF)
endif()

# Export compile commands used for custom targets
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
  message_colored(STATUS "No build type specified; using ${CMAKE_BUILD_TYPE}" 33)
endif()
string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)
