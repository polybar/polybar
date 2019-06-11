#
# Configure src
#

# Source tree {{{

get_filename_component(generated_sources_dir  ${CMAKE_BINARY_DIR}/generated-sources   ABSOLUTE)
get_filename_component(include_dir            ${CMAKE_SOURCE_DIR}/include             ABSOLUTE)
get_filename_component(src_dir                ${CMAKE_CURRENT_SOURCE_DIR}             ABSOLUTE)

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

set(XCOMPOSITE_SOURCES ${src_dir}/x11/extensions/composite.cpp)

set(XCURSOR_SOURCES ${src_dir}/x11/cursor.cpp)

set(XKB_SOURCES
  ${src_dir}/modules/xkeyboard.cpp
  ${src_dir}/x11/extensions/xkb.cpp
)

set(XRM_SOURCES ${src_dir}/x11/xresources.cpp)

set(POLY_SOURCES
  ${src_dir}/cairo/utils.cpp
  ${src_dir}/components/bar.cpp
  ${src_dir}/components/builder.cpp
  ${src_dir}/components/command_line.cpp
  ${src_dir}/components/config.cpp
  ${src_dir}/components/controller.cpp
  ${src_dir}/components/ipc.cpp
  ${src_dir}/components/logger.cpp
  ${src_dir}/components/parser.cpp
  ${src_dir}/components/renderer.cpp
  ${src_dir}/components/screen.cpp
  ${src_dir}/components/taskqueue.cpp
  ${src_dir}/drawtypes/animation.cpp
  ${src_dir}/drawtypes/iconset.cpp
  ${src_dir}/drawtypes/label.cpp
  ${src_dir}/drawtypes/progressbar.cpp
  ${src_dir}/drawtypes/ramp.cpp
  ${src_dir}/events/signal_emitter.cpp
  ${src_dir}/events/signal_receiver.cpp
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
  ${src_dir}/utils/bspwm.cpp
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
  ${src_dir}/utils/throttle.cpp
  ${src_dir}/x11/atoms.cpp
  ${src_dir}/x11/background_manager.cpp
  ${src_dir}/x11/connection.cpp
  ${src_dir}/x11/ewmh.cpp
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
  $<$<BOOL:${WITH_XCOMPOSITE}>:${XCOMPOSITE_SOURCES}>
  $<$<BOOL:${WITH_XCURSOR}>:${XCURSOR_SOURCES}>
  $<$<BOOL:${WITH_XKB}>:${XKB_SOURCES}>
  $<$<BOOL:${WITH_XRM}>:${XRM_SOURCES}>
)

# }}}

# Target: polybar {{{

add_library(poly STATIC ${POLY_SOURCES})
target_include_directories(poly PUBLIC ${include_dir} ${generated_sources_dir})
target_link_libraries(poly PUBLIC
  Threads::Threads
  Cairo::CairoFC
  moodycamel
  xpp
  $<$<TARGET_EXISTS:i3ipc++>:i3ipc++>
  $<$<TARGET_EXISTS:ALSA::ALSA>:ALSA::ALSA>
  $<$<TARGET_EXISTS:CURL::libcurl>:CURL::libcurl>
  $<$<TARGET_EXISTS:LibMPDClient::LibMPDClient>:LibMPDClient::LibMPDClient>
  $<$<TARGET_EXISTS:LibNlGenl3::LibNlGenl3>:LibNlGenl3::LibNlGenl3>
  $<$<TARGET_EXISTS:Libiw::Libiw>:Libiw::Libiw>
  $<$<TARGET_EXISTS:LibPulse::LibPulse>:LibPulse::LibPulse>
  $<$<TARGET_EXISTS:Xcb::RANDR>:Xcb::RANDR>
  $<$<TARGET_EXISTS:Xcb::COMPOSITE>:Xcb::COMPOSITE>
  $<$<TARGET_EXISTS:Xcb::XKB>:Xcb::XKB>
  $<$<TARGET_EXISTS:Xcb::CURSOR>:Xcb::CURSOR>
  $<$<TARGET_EXISTS:Xcb::XRM>:Xcb::XRM>
  $<$<TARGET_EXISTS:LibInotify::LibInotify>:LibInotify::LibInotify>
)

target_compile_options(poly PUBLIC $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)
set_target_properties(poly PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs)

add_executable(polybar main.cpp)
target_link_libraries(polybar poly)
set_target_properties(poly PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

install(TARGETS polybar
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT runtime)

# }}}
# Target: polybar-msg {{{

if(BUILD_IPC_MSG)
  add_executable(polybar-msg
    ipc.cpp
    utils/env.cpp
    utils/file.cpp
    utils/string.cpp)
  target_include_directories(polybar-msg PRIVATE ${dirs})
  target_compile_options(polybar-msg PUBLIC $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)

  install(TARGETS polybar-msg
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT runtime)

endif()

# }}}

# Export source file list so that it can be used for test compilation
set(files ${files} PARENT_SCOPE)
