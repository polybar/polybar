#pragma once

#include "settings.hpp"

#if WITH_XRANDR
#include "x11/extensions/randr.hpp"
#endif
#include "x11/extensions/composite.hpp"
#if WITH_XKB
#include "x11/extensions/xkb.hpp"
#endif
