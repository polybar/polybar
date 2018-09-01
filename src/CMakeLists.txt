#
# Configure src
#


# Source tree {{{

add_subdirectory(cairo)
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
    components/bar.cpp
    components/builder.cpp
    components/command_line.cpp
    components/config.cpp
    components/controller.cpp
    components/ipc.cpp
    components/logger.cpp
    components/parser.cpp
    components/renderer.cpp
    components/screen.cpp
    components/taskqueue.cpp
  INCLUDE_DIRS
    ${dirs}
  TARGET_DEPENDS
    polybar-modules-internal_shared
    polybar-cairo_static
    polybar-drawtypes_static
    polybar-events_static
    polybar-utils_static
    polybar-x11_static
  RAW_DEPENDS
    ${libs}
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
