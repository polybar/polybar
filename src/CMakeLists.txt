#
# Configure src
#


# Source tree {{{

add_subdirectory(cairo)
add_subdirectory(components)
add_subdirectory(drawtypes)
add_subdirectory(events)
add_subdirectory(modules)
add_subdirectory(utils)
add_subdirectory(x11)

# }}}

# Target: polybar {{{

make_executable(polybar
  SOURCES
    main.cpp
    ${files}
  INCLUDE_DIRS
    ${dirs}
  TARGET_DEPENDS
    polybar-modules-internal_shared
  RAW_DEPENDS
    ${libs}
    ${CMAKE_DL_LIBS}
    Threads::Threads)

target_compile_options(polybar PUBLIC $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)

# }}}
# Target: polybar-msg {{{

if(BUILD_IPC_MSG)
  make_executable(polybar-msg
    SOURCES
      ipc.cpp
      utils/env.cpp
      utils/file.cpp
      utils/string.cpp)
  target_compile_options(polybar-msg PUBLIC $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)
endif()

# }}}

# Export source file list so that it can be used for test compilation
set(files ${files} PARENT_SCOPE)
set(libs ${libs} PARENT_SCOPE)
set(dirs ${dirs} PARENT_SCOPE)
