#pragma once

/**
 * Header file to include the headers for all modules.
 */

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
#include "modules/tray.hpp"
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
