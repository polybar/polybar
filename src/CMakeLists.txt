#
# Create library and executable
#
set(library_name lemonlib)
set(executable_name lemonbuddy)

file(GLOB_RECURSE headers RELATIVE ${PROJECT_SOURCE_DIR}/include *.h[p]*)
file(GLOB_RECURSE sources RELATIVE ${PROJECT_SOURCE_DIR}/src *.c[p]*)
list(REMOVE_ITEM sources main.cpp)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

make_library(${library_name} STATIC
  HEADER_INSTALL_DIR
    lemonbuddy
  HEADERS
    ${headers}
  SOURCES
    ${sources})

make_executable(${executable_name}
  SOURCES main.cpp
  TARGET_DEPENDS ${library_name}_static)

target_compile_definitions(lemonbuddy PUBLIC
    ${X11_XCB_DEFINITIONS}
    ${XCB_DEFINITIONS})

configure_file(
  ${PROJECT_SOURCE_DIR}/include/config.hpp.cmake
  ${CMAKE_SOURCE_DIR}/include/config.hpp
  ESCAPE_QUOTES @ONLY)

set(app_binary ${PROJECT_SOURCE_DIR}/bin/${executable_name} PARENT_SCOPE)
set(app_libraries ${XPP_LIBRARIES} PARENT_SCOPE)
set(app_include_dirs ${PROJECT_SOURCE_DIR}/include PARENT_SCOPE)
