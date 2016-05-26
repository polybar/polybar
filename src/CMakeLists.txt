if(ENABLE_ALSA)
  set(SOURCE_FILES_ALSA
    "src/interfaces/alsa.cpp"
    "src/modules/volume.cpp")
endif()

if(ENABLE_MPD)
  set(SOURCE_FILES_MPD
    "src/interfaces/mpd.cpp"
    "src/modules/mpd.cpp")
endif()

if(ENABLE_NETWORK)
  set(SOURCE_FILES_NETWORK
    "src/interfaces/net.cpp"
    "src/modules/network.cpp")
endif()

if(ENABLE_I3)
  set(SOURCE_FILES_I3 "src/modules/i3.cpp")
endif()

set(SOURCE_FILES
  ${SOURCE_FILES_ALSA}
  ${SOURCE_FILES_I3}
  ${SOURCE_FILES_MPD}
  ${SOURCE_FILES_NETWORK}
  "src/drawtypes/animation.cpp"
  "src/drawtypes/bar.cpp"
  "src/drawtypes/icon.cpp"
  "src/drawtypes/label.cpp"
  "src/drawtypes/ramp.cpp"
  "src/modules/backlight.cpp"
  "src/modules/base.cpp"
  "src/modules/battery.cpp"
  "src/modules/bspwm.cpp"
  "src/modules/counter.cpp"
  "src/modules/cpu.cpp"
  "src/modules/date.cpp"
  "src/modules/memory.cpp"
  "src/modules/menu.cpp"
  "src/modules/script.cpp"
  "src/modules/text.cpp"
  # "src/modules/torrent.cpp"
  "src/services/builder.cpp"
  "src/services/command.cpp"
  "src/services/inotify.cpp"
  "src/services/logger.cpp"
  # "src/services/store.cpp"
  "src/utils/cli.cpp"
  "src/utils/config.cpp"
  "src/utils/io.cpp"
  "src/utils/proc.cpp"
  "src/utils/string.cpp"
  "src/utils/timer.cpp"
  "src/utils/xlib.cpp"
  "src/bar.cpp"
  "src/eventloop.cpp"
  "src/lemonbuddy.cpp"
  "src/registry.cpp"
  PARENT_SCOPE
)
