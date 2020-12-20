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
#if ENABLE_DWM
#include "modules/dwm.hpp"
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
#include "modules/unsupported.hpp"

POLYBAR_NS

using namespace modules;

namespace {
  module_interface* make_module(string&& name, const bar_settings& bar, string module_name, const logger& m_log) {
    if (name == counter_module::TYPE) {
      return new counter_module(bar, move(module_name));
    } else if (name == backlight_module::TYPE) {
      return new backlight_module(bar, move(module_name));
    } else if (name == battery_module::TYPE) {
      return new battery_module(bar, move(module_name));
    } else if (name == bspwm_module::TYPE) {
      return new bspwm_module(bar, move(module_name));
    } else if (name == cpu_module::TYPE) {
      return new cpu_module(bar, move(module_name));
    } else if (name == date_module::TYPE) {
      return new date_module(bar, move(module_name));
    } else if (name == dwm_module::TYPE) {
      return new dwm_module(bar, move(module_name));
    } else if (name == github_module::TYPE) {
      return new github_module(bar, move(module_name));
    } else if (name == fs_module::TYPE) {
      return new fs_module(bar, move(module_name));
    } else if (name == memory_module::TYPE) {
      return new memory_module(bar, move(module_name));
    } else if (name == i3_module::TYPE) {
      return new i3_module(bar, move(module_name));
    } else if (name == mpd_module::TYPE) {
      return new mpd_module(bar, move(module_name));
    } else if (name == "internal/volume") {
      m_log.warn("internal/volume is deprecated, use %s instead", string(alsa_module::TYPE));
      return new alsa_module(bar, move(module_name));
    } else if (name == alsa_module::TYPE) {
      return new alsa_module(bar, move(module_name));
    } else if (name == pulseaudio_module::TYPE) {
      return new pulseaudio_module(bar, move(module_name));
    } else if (name == network_module::TYPE) {
      return new network_module(bar, move(module_name));
#if DEBUG
    } else if (name == systray_module::TYPE) {
      return new systray_module(bar, move(module_name));
#endif
    } else if (name == temperature_module::TYPE) {
      return new temperature_module(bar, move(module_name));
    } else if (name == xbacklight_module::TYPE) {
      return new xbacklight_module(bar, move(module_name));
    } else if (name == xkeyboard_module::TYPE) {
      return new xkeyboard_module(bar, move(module_name));
    } else if (name == xwindow_module::TYPE) {
      return new xwindow_module(bar, move(module_name));
    } else if (name == xworkspaces_module::TYPE) {
      return new xworkspaces_module(bar, move(module_name));
    } else if (name == text_module::TYPE) {
      return new text_module(bar, move(module_name));
    } else if (name == script_module::TYPE) {
      return new script_module(bar, move(module_name));
    } else if (name == menu_module::TYPE) {
      return new menu_module(bar, move(module_name));
    } else if (name == ipc_module::TYPE) {
      return new ipc_module(bar, move(module_name));
    } else {
      throw application_error("Unknown module: " + name);
    }
  }
}  // namespace

POLYBAR_NS_END
