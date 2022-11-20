option(ENABLE_CCACHE "Enable ccache support" ON)
if(ENABLE_CCACHE)
  find_program(BIN_CCACHE ccache)
  mark_as_advanced(BIN_CCACHE)

  if(NOT BIN_CCACHE)
    message_colored(STATUS "Couldn't locate ccache, disabling ccache..." "33")
  else()
    # Enable only if the binary is found
    message_colored(STATUS "Using compiler cache ${BIN_CCACHE}" "32")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${BIN_CCACHE} CACHE STRING "")
  endif()
endif()

option(CXXLIB_CLANG "Link against libc++" OFF)
option(CXXLIB_GCC "Link against stdlibc++" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(POLYBAR_FLAGS "" CACHE STRING "C++ compiler flags used for compiling polybar")

list(APPEND cxx_base -Wall -Wextra -Wpedantic -Wdeprecated-copy-dtor)
list(APPEND cxx_debug -DDEBUG -g2 -Og)
list(APPEND cxx_minsizerel "")
list(APPEND cxx_sanitize ${cxx_debug} -O0 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls)
list(APPEND cxx_coverage ${cxx_debug} --coverage)

list(APPEND cxx_linker_base "")
list(APPEND cxx_linker_minsizerel "")

# Compiler flags
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-Wsuggest-override" HAS_SUGGEST_OVERRIDE)
if (HAS_SUGGEST_OVERRIDE)
  list(APPEND cxx_base -Wsuggest-override)
endif()
unset(HAS_SUGGEST_OVERRIDE)

if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  # Need dprintf() for FreeBSD 11.1 and older
  # libinotify uses c99 extension, so suppress this error
  list(APPEND cxx_base -D_WITH_DPRINTF -Wno-c99-extensions)
  # Ensures that libraries from dependencies in LOCALBASE are used
  list(APPEND cxx_linker_base -L/usr/local/lib)
endif()

# Check compiler
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  list(APPEND cxx_base -Wno-error=parentheses-equality -Wno-zero-length-array)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "3.4.0")
    message_colored(FATAL_ERROR "Compiler not supported (Requires clang-3.4+ or gcc-5.1+)" 31)
  else()
    message_colored(STATUS "Using supported compiler ${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}" 32)
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  list(APPEND cxx_minsizerel -fdata-sections -ffunction-sections -flto)
  list(APPEND cxx_linker_minsizerel -Wl,--gc-sections)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.1.0")
    message_colored(FATAL_ERROR "Compiler not supported (Requires clang-3.4+ or gcc-5.1+)" 31)
  else()
    message_colored(STATUS "Using supported compiler ${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}" 32)
  endif()
else()
  message_colored(WARNING "Using unsupported compiler ${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION} !" 31)
endif()

# Set compiler and linker flags for preferred C++ library
if(CXXLIB_CLANG)
  message_colored(STATUS "Linking against libc++" 32)
  list(APPEND cxx_base -stdlib=libc++)
  list(APPEND cxx_linker_base -lc++ -lc++abi)
elseif(CXXLIB_GCC)
  message_colored(STATUS "Linking against libstdc++" 32)
  list(APPEND cxx_linker_base -lstdc++)
endif()

SET(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS_COVERAGE}")
SET(CMAKE_EXE_LINKER_FLAGS_COVERAGE "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${CMAKE_EXE_LINKER_FLAGS_COVERAGE}")
SET(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} ${CMAKE_SHARED_LINKER_FLAGS_COVERAGE}")

list(APPEND cxx_flags ${cxx_base})
list(APPEND cxx_linker_flags ${cxx_linker_base})

if (CMAKE_BUILD_TYPE_UPPER STREQUAL "DEBUG")
  list(APPEND cxx_flags ${cxx_debug})
elseif (CMAKE_BUILD_TYPE_UPPER STREQUAL "MINSIZEREL")
  list(APPEND cxx_flags ${cxx_minsizerel})
  list(APPEND cxx_linker_flags ${cxx_linker_minsizerel})
elseif (CMAKE_BUILD_TYPE_UPPER STREQUAL "SANITIZE")
  list(APPEND cxx_flags ${cxx_sanitize})
elseif (CMAKE_BUILD_TYPE_UPPER STREQUAL "COVERAGE")
  list(APPEND cxx_flags ${cxx_coverage})
endif()

string(REPLACE " " ";" polybar_flags_list "${POLYBAR_FLAGS}")
list(APPEND cxx_flags ${polybar_flags_list})

list(APPEND cxx_linker_flags ${cxx_flags})

string(REPLACE ";" " " cxx_flags_str "${cxx_flags}")
string(REPLACE ";" " " cxx_linker_flags_str "${cxx_linker_flags}")

# TODO use target_link_options once min cmake version is >= 3.13
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${cxx_linker_flags_str}")
