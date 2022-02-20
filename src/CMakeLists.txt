#
# Configure src
#

get_include_dirs(includes_dir)
get_sources_dirs(src_dir)

# Source tree {{{

set(ALSA_SOURCES
  ${src_dir}/adapters/alsa/control.cpp
  ${src_dir}/adapters/alsa/mixer.cpp
  ${src_dir}/modules/alsa.cpp
  )

set(GITHUB_SOURCES ${src_dir}/modules/github.cpp ${src_dir}/utils/http.cpp)

set(I3_SOURCES
  ${src_dir}/modules/i3.cpp
  ${src_dir}/utils/i3.cpp
  )

set(MPD_SOURCES
  ${src_dir}/adapters/mpd.cpp
  ${src_dir}/modules/mpd.cpp
  )

set(NETWORK_SOURCES
  ${src_dir}/adapters/net.cpp
  ${src_dir}/modules/network.cpp
  $<IF:$<BOOL:${WITH_LIBNL}>,${src_dir}/adapters/net_nl.cpp,${src_dir}/adapters/net_iw.cpp>
  )

set(PULSEAUDIO_SOURCES
  ${src_dir}/adapters/pulseaudio.cpp
  ${src_dir}/modules/pulseaudio.cpp
  )

set(XCURSOR_SOURCES ${src_dir}/x11/cursor.cpp)

set(XKB_SOURCES
  ${src_dir}/modules/xkeyboard.cpp
  ${src_dir}/x11/extensions/xkb.cpp
  )

set(XRM_SOURCES ${src_dir}/x11/xresources.cpp)

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/settings.cpp.cmake
  ${CMAKE_BINARY_DIR}/generated-sources/settings.cpp
  ESCAPE_QUOTES)

set(POLY_SOURCES
  ${CMAKE_BINARY_DIR}/generated-sources/settings.cpp

  ${src_dir}/adapters/script_runner.cpp

  ${src_dir}/cairo/utils.cpp

  ${src_dir}/components/bar.cpp
  ${src_dir}/components/builder.cpp
  ${src_dir}/components/command_line.cpp
  ${src_dir}/components/config.cpp
  ${src_dir}/components/config_parser.cpp
  ${src_dir}/components/controller.cpp
  ${src_dir}/components/logger.cpp
  ${src_dir}/components/renderer.cpp
  ${src_dir}/components/screen.cpp
  ${src_dir}/components/eventloop.cpp

  ${src_dir}/drawtypes/animation.cpp
  ${src_dir}/drawtypes/iconset.cpp
  ${src_dir}/drawtypes/layouticonset.cpp
  ${src_dir}/drawtypes/label.cpp
  ${src_dir}/drawtypes/progressbar.cpp
  ${src_dir}/drawtypes/ramp.cpp

  ${src_dir}/events/signal_emitter.cpp
  ${src_dir}/events/signal_receiver.cpp

  ${src_dir}/ipc/ipc.cpp
  ${src_dir}/ipc/decoder.cpp
  ${src_dir}/ipc/encoder.cpp
  ${src_dir}/ipc/util.cpp

  ${src_dir}/modules/backlight.cpp
  ${src_dir}/modules/battery.cpp
  ${src_dir}/modules/bspwm.cpp
  ${src_dir}/modules/counter.cpp
  ${src_dir}/modules/cpu.cpp
  ${src_dir}/modules/date.cpp
  ${src_dir}/modules/fs.cpp
  ${src_dir}/modules/ipc.cpp
  ${src_dir}/modules/memory.cpp
  ${src_dir}/modules/menu.cpp
  ${src_dir}/modules/meta/base.cpp
  ${src_dir}/modules/script.cpp
  ${src_dir}/modules/systray.cpp
  ${src_dir}/modules/temperature.cpp
  ${src_dir}/modules/text.cpp
  ${src_dir}/modules/xbacklight.cpp
  ${src_dir}/modules/xwindow.cpp
  ${src_dir}/modules/xworkspaces.cpp

  ${src_dir}/tags/action_context.cpp
  ${src_dir}/tags/context.cpp
  ${src_dir}/tags/dispatch.cpp
  ${src_dir}/tags/parser.cpp

  ${src_dir}/utils/actions.cpp
  ${src_dir}/utils/action_router.cpp
  ${src_dir}/utils/bspwm.cpp
  ${src_dir}/utils/color.cpp
  ${src_dir}/utils/command.cpp
  ${src_dir}/utils/concurrency.cpp
  ${src_dir}/utils/env.cpp
  ${src_dir}/utils/factory.cpp
  ${src_dir}/utils/file.cpp
  ${src_dir}/utils/inotify.cpp
  ${src_dir}/utils/io.cpp
  ${src_dir}/utils/process.cpp
  ${src_dir}/utils/socket.cpp
  ${src_dir}/utils/string.cpp
  ${src_dir}/utils/units.cpp

  ${src_dir}/x11/atoms.cpp
  ${src_dir}/x11/background_manager.cpp
  ${src_dir}/x11/connection.cpp
  ${src_dir}/x11/ewmh.cpp
  ${src_dir}/x11/extensions/composite.cpp
  ${src_dir}/x11/extensions/randr.cpp
  ${src_dir}/x11/icccm.cpp
  ${src_dir}/x11/registry.cpp
  ${src_dir}/x11/tray_client.cpp
  ${src_dir}/x11/tray_manager.cpp
  ${src_dir}/x11/window.cpp
  ${src_dir}/x11/winspec.cpp
  ${src_dir}/x11/xembed.cpp

  $<$<BOOL:${ENABLE_ALSA}>:${ALSA_SOURCES}>
  $<$<BOOL:${ENABLE_CURL}>:${GITHUB_SOURCES}>
  $<$<BOOL:${ENABLE_I3}>:${I3_SOURCES}>
  $<$<BOOL:${ENABLE_MPD}>:${MPD_SOURCES}>
  $<$<BOOL:${ENABLE_NETWORK}>:${NETWORK_SOURCES}>
  $<$<BOOL:${ENABLE_PULSEAUDIO}>:${PULSEAUDIO_SOURCES}>
  $<$<BOOL:${WITH_XCURSOR}>:${XCURSOR_SOURCES}>
  $<$<BOOL:${WITH_XKB}>:${XKB_SOURCES}>
  $<$<BOOL:${WITH_XRM}>:${XRM_SOURCES}>
  )

# }}}

# Target poly {{{
add_library(poly STATIC EXCLUDE_FROM_ALL ${POLY_SOURCES})
target_include_directories(poly PUBLIC ${includes_dir})
target_link_libraries(poly PUBLIC
  Threads::Threads
  Cairo::CairoFC
  xpp
  LibUV::LibUV
  )

if (TARGET i3ipc++)
  target_link_libraries(poly PUBLIC i3ipc++)
endif()

if (TARGET ALSA::ALSA)
  target_link_libraries(poly PUBLIC ALSA::ALSA)
endif()

if (TARGET CURL::libcurl)
  target_link_libraries(poly PUBLIC CURL::libcurl)
endif()

if (TARGET LibMPDClient::LibMPDClient)
  target_link_libraries(poly PUBLIC LibMPDClient::LibMPDClient)
endif()

if (TARGET LibNlGenl3::LibNlGenl3)
  target_link_libraries(poly PUBLIC LibNlGenl3::LibNlGenl3)
endif()

if (TARGET Libiw::Libiw)
  target_link_libraries(poly PUBLIC Libiw::Libiw)
endif()

if (TARGET LibPulse::LibPulse)
  target_link_libraries(poly PUBLIC LibPulse::LibPulse)
endif()

if (TARGET Xcb::RANDR)
  target_link_libraries(poly PUBLIC Xcb::RANDR)
endif()

if (TARGET Xcb::COMPOSITE)
  target_link_libraries(poly PUBLIC Xcb::COMPOSITE)
endif()

if (TARGET Xcb::XKB)
  target_link_libraries(poly PUBLIC Xcb::XKB)
endif()

if (TARGET Xcb::CURSOR)
  target_link_libraries(poly PUBLIC Xcb::CURSOR)
endif()

if (TARGET Xcb::XRM)
  target_link_libraries(poly PUBLIC Xcb::XRM)
endif()

if (TARGET LibInotify::LibInotify)
  target_link_libraries(poly PUBLIC LibInotify::LibInotify)
endif()

target_compile_options(poly PUBLIC ${cxx_flags})
set_target_properties(poly PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs)
# }}}

# Target: polybar {{{
if(BUILD_POLYBAR)
  add_executable(polybar main.cpp)
  target_link_libraries(polybar poly)
  target_compile_options(polybar PUBLIC ${cxx_flags})
  set_target_properties(polybar PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

  install(TARGETS polybar
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT runtime)
endif()
# }}}

# Target: polybar-msg {{{
if(BUILD_POLYBAR_MSG)
  add_executable(polybar-msg polybar-msg.cpp)
  target_link_libraries(polybar-msg poly)
  target_include_directories(polybar-msg PRIVATE ${includes_dir})
  target_compile_options(polybar-msg PUBLIC ${cxx_flags})

  install(TARGETS polybar-msg
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT runtime)
endif()
# }}}
