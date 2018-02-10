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
#if DEBUG
#include "modules/systray.hpp"
#endif
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
#include "modules/alsa.hpp"
#endif
#if ENABLE_PULSEAUDIO
#include "modules/pulseaudio.hpp"
#endif
#if ENABLE_CURL
#include "modules/github.hpp"
#endif
#if ENABLE_XKEYBOARD
#include "modules/xkeyboard.hpp"
#endif
#if not(ENABLE_I3 && ENABLE_MPD && ENABLE_NETWORK && ENABLE_ALSA && ENABLE_PULSEAUDIO && ENABLE_CURL && ENABLE_XKEYBOARD)
#include "modules/unsupported.hpp"
#endif

POLYBAR_NS

using namespace modules;

namespace {
  module_interface* make_module(string&& name, const bar_settings& bar, string module_name, const logger& m_log) {
    if (name == "internal/counter") {
      return new counter_module(bar, move(module_name));
    } else if (name == "internal/backlight") {
      return new backlight_module(bar, move(module_name));
    } else if (name == "internal/battery") {
      return new battery_module(bar, move(module_name));
    } else if (name == "internal/bspwm") {
      return new bspwm_module(bar, move(module_name));
    } else if (name == "internal/cpu") {
      return new cpu_module(bar, move(module_name));
    } else if (name == "internal/date") {
      return new date_module(bar, move(module_name));
    } else if (name == "internal/github") {
      return new github_module(bar, move(module_name));
    } else if (name == "internal/fs") {
      return new fs_module(bar, move(module_name));
    } else if (name == "internal/memory") {
      return new memory_module(bar, move(module_name));
    } else if (name == "internal/i3") {
      return new i3_module(bar, move(module_name));
    } else if (name == "internal/mpd") {
      return new mpd_module(bar, move(module_name));
    } else if (name == "internal/volume") {
      m_log.warn("internal/volume is deprecated, use internal/alsa instead");
      return new alsa_module(bar, move(module_name));
    } else if (name == "internal/alsa") {
      return new alsa_module(bar, move(module_name));
    } else if (name == "internal/pulseaudio") {
      return new pulseaudio_module(bar, move(module_name));
    } else if (name == "internal/network") {
      return new network_module(bar, move(module_name));
#if DEBUG
    } else if (name == "internal/systray") {
      return new systray_module(bar, move(module_name));
#endif
    } else if (name == "internal/temperature") {
      return new temperature_module(bar, move(module_name));
    } else if (name == "internal/xbacklight") {
      return new xbacklight_module(bar, move(module_name));
    } else if (name == "internal/xkeyboard") {
      return new xkeyboard_module(bar, move(module_name));
    } else if (name == "internal/xwindow") {
      return new xwindow_module(bar, move(module_name));
    } else if (name == "internal/xworkspaces") {
      return new xworkspaces_module(bar, move(module_name));
    } else if (name == "custom/text") {
      return new text_module(bar, move(module_name));
    } else if (name == "custom/script") {
      return new script_module(bar, move(module_name));
    } else if (name == "custom/menu") {
      return new menu_module(bar, move(module_name));
    } else if (name == "custom/ipc") {
      return new ipc_module(bar, move(module_name));
    } else {
      throw application_error("Unknown module: " + name);
    }
  }
}

POLYBAR_NS_END
