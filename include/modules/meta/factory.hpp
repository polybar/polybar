#pragma once

#include "common.hpp"

#include "modules/backlight.hpp"
#include "modules/battery.hpp"
#include "modules/bspwm.hpp"
#include "modules/counter.hpp"
#include "modules/cpu.hpp"
#include "modules/date.hpp"
#include "modules/fs.hpp"
#include "modules/ipc.hpp"
#include "modules/memory.hpp"
#include "modules/menu.hpp"
#include "modules/meta/base.hpp"
#include "modules/script.hpp"
#include "modules/temperature.hpp"
#include "modules/text.hpp"
#include "modules/xbacklight.hpp"
#include "modules/xwindow.hpp"
#include "modules/xworkspaces.hpp"
#if ENABLE_I3
#include "modules/i3.hpp"
#endif
#if ENABLE_MPD
#include "modules/mpd.hpp"
#endif
#if ENABLE_NETWORK
#include "modules/network.hpp"
#endif
#if ENABLE_ALSA
#include "modules/volume.hpp"
#endif
#if WITH_XKB
#include "modules/xkeyboard.hpp"
#endif
#if not(ENABLE_I3 && ENABLE_MPD && ENABLE_NETWORK && ENABLE_ALSA && WITH_XKB)
#include "modules/unsupported.hpp"
#endif

POLYBAR_NS

using namespace modules;

namespace {
  template <typename... Args>
  module_interface* make_module(string&& name, Args&&... args) {
    if (name == "internal/counter") {
      return new counter_module(forward<Args>(args)...);
    } else if (name == "internal/backlight") {
      return new backlight_module(forward<Args>(args)...);
    } else if (name == "internal/battery") {
      return new battery_module(forward<Args>(args)...);
    } else if (name == "internal/bspwm") {
      return new bspwm_module(forward<Args>(args)...);
    } else if (name == "internal/cpu") {
      return new cpu_module(forward<Args>(args)...);
    } else if (name == "internal/date") {
      return new date_module(forward<Args>(args)...);
    } else if (name == "internal/fs") {
      return new fs_module(forward<Args>(args)...);
    } else if (name == "internal/memory") {
      return new memory_module(forward<Args>(args)...);
    } else if (name == "internal/i3") {
      return new i3_module(forward<Args>(args)...);
    } else if (name == "internal/mpd") {
      return new mpd_module(forward<Args>(args)...);
    } else if (name == "internal/volume") {
      return new volume_module(forward<Args>(args)...);
    } else if (name == "internal/network") {
      return new network_module(forward<Args>(args)...);
    } else if (name == "internal/temperature") {
      return new temperature_module(forward<Args>(args)...);
    } else if (name == "internal/xbacklight") {
      return new xbacklight_module(forward<Args>(args)...);
    } else if (name == "internal/xkeyboard") {
      return new xkeyboard_module(forward<Args>(args)...);
    } else if (name == "internal/xwindow") {
      return new xwindow_module(forward<Args>(args)...);
    } else if (name == "internal/xworkspaces") {
      return new xworkspaces_module(forward<Args>(args)...);
    } else if (name == "custom/text") {
      return new text_module(forward<Args>(args)...);
    } else if (name == "custom/script") {
      return new script_module(forward<Args>(args)...);
    } else if (name == "custom/menu") {
      return new menu_module(forward<Args>(args)...);
    } else if (name == "custom/ipc") {
      return new ipc_module(forward<Args>(args)...);
    } else {
      throw application_error("Unknown module: " + name);
    }
  }
}

POLYBAR_NS_END
