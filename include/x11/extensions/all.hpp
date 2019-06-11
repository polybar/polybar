#pragma once

#include "settings.hpp"

#if WITH_XRANDR
#include "x11/extensions/randr.hpp"
#endif
#if WITH_XCOMPOSITE
#include "x11/extensions/composite.hpp"
#endif
#if WITH_XKB
#include "x11/extensions/xkb.hpp"
#endif
