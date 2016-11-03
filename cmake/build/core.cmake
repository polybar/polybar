#
# Core setup
#

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands used for custom targets
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  message_colored(STATUS "No build type specified; using Release" 33)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# Generic compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

# Debug specific compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pedantic-errors")

# Release specific compiler flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Compiler specific flags
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL Clang)
  message_colored(STATUS "Using supported compiler ${CMAKE_CXX_COMPILER_ID}" 32)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL GNU)
  message_colored(STATUS "Using supported compiler ${CMAKE_CXX_COMPILER_ID}" 32)
else()
  message_colored(WARNING "Using unsupported compiler ${CMAKE_CXX_COMPILER_ID} !" 31)
endif()

# Set compiler and linker flags for preferred C++ library
if(CXXLIB_CLANG)
  message_colored(STATUS "Linking against libc++" 32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++ -lc++abi")
elseif(CXXLIB_GCC)
  message_colored(STATUS "Linking against libstdc++" 32)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++")
else()
  message_colored(STATUS "No preferred c++lib specified... linking against system default" 33)
endif()

if(ENABLE_CCACHE)
  require_binary(ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${BINPATH_ccache})
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${BINPATH_ccache})
endif()
