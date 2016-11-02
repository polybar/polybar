#
# Create library and executable
#

set(BINARY_NAME ${PROJECT_NAME})
set(LIBRARY_NAME lib${PROJECT_NAME})

file(GLOB_RECURSE HEADERS RELATIVE ${PROJECT_SOURCE_DIR}/include *.h[p]*)
file(GLOB_RECURSE SOURCES RELATIVE ${PROJECT_SOURCE_DIR}/src *.c[p]*)
list(REMOVE_ITEM SOURCES main.cpp)

configure_file(
  ${PROJECT_SOURCE_DIR}/include/config.hpp.cmake
  ${CMAKE_SOURCE_DIR}/include/config.hpp
  ESCAPE_QUOTES @ONLY)

# Target: main library {{{

make_library(${LIBRARY_NAME} STATIC
  HEADER_INSTALL_DIR
    ${PROJECT_NAME}
  HEADERS
    ${HEADERS}
  SOURCES
    ${SOURCES})

target_include_directories(${LIBRARY_NAME}_static PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# }}}
# Target: main executable {{{

make_executable(${BINARY_NAME}
  SOURCES main.cpp
  TARGET_DEPENDS ${LIBRARY_NAME}_static)

# }}}

# Link dependencies {{{

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(Freetype REQUIRED Freetype2)
find_package(X11 REQUIRED COMPONENTS Xft Xutil)
find_package(X11_XCB REQUIRED)

find_package(PkgConfig)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)

target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${BOOST_LIBRARIES})
target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${LIBRARY_NAME}_static PUBLIC Threads::Threads)
target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${X11_LIBRARIES})
target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${X11_X11_LIB})
target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${X11_XCB_LIB})
target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${X11_Xft_LIB})
target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${FREETYPE_LIBRARIES})
target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${FONTCONFIG_LIBRARIES})

target_include_directories(${LIBRARY_NAME}_static PUBLIC ${BOOST_INCLUDE_DIR})
target_include_directories(${LIBRARY_NAME}_static PUBLIC ${FONTCONFIG_INCLUDE_DIRS})
target_include_directories(${LIBRARY_NAME}_static PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(${LIBRARY_NAME}_static PUBLIC ${PROJECT_SOURCE_DIR}/lib/boost/include)
target_include_directories(${LIBRARY_NAME}_static PUBLIC ${PROJECT_SOURCE_DIR}/lib/concurrentqueue/include)

target_compile_definitions(${BINARY_NAME} PUBLIC
    ${X11_XCB_DEFINITIONS}
    ${XCB_DEFINITIONS})

# xpp library
set(XCB_PROTOS xproto)
if(ENABLE_RANDR_EXT)
  set(XCB_PROTOS "${XCB_PROTOS}" randr)
endif()
if(ENABLE_RENDER_EXT)
  set(XCB_PROTOS "${XCB_PROTOS}" render)
endif()
if(ENABLE_DAMAGE_EXT)
  set(XCB_PROTOS "${XCB_PROTOS}" damage)
endif()
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/xpp ${PROJECT_BINARY_DIR}/lib/xpp)
target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${XPP_LIBRARIES})

# }}}

# Optional dependency: alsalib {{{

if(ENABLE_ALSA)
  find_package(ALSA REQUIRED)
  target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${ALSA_LIBRARY})
endif()

# }}}
# Optional dependency: libmpdclient {{{

if(ENABLE_MPD)
  find_package(LibMPDClient REQUIRED)
  target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${LIBMPDCLIENT_LIBRARIES})
endif()

# }}}
# Optional dependency: libiw {{{

if(ENABLE_NETWORK)
  find_package(Libiw REQUIRED)
  target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${LIBIW_LIBRARY})
endif()

# }}}
# Optional dependency: i3ipcpp {{{

if(ENABLE_I3)
  add_subdirectory(${PROJECT_SOURCE_DIR}/lib/i3ipcpp ${PROJECT_BINARY_DIR}/lib/i3ipcpp)
  target_include_directories(${LIBRARY_NAME}_static PUBLIC ${I3IPCPP_INCLUDE_DIRS})
  target_link_libraries(${LIBRARY_NAME}_static PUBLIC ${I3IPCPP_LIBRARIES})
endif()

# }}}

# Export target details {{{

set(APP_BINARY ${PROJECT_SOURCE_DIR}/bin/${BINARY_NAME} PARENT_SCOPE)
set(APP_LIBRARIES ${LIBRARY_NAME}_static ${XPP_LIBRARY} PARENT_SCOPE)
set(APP_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/lib/boost/include
  ${PROJECT_SOURCE_DIR}/lib/concurrentqueue/include
  ${XPP_INCLUDE_DIRS}
  PARENT_SCOPE)

# }}}
