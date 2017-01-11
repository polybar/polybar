#pragma once

#include "settings.hpp"

#if WITH_XDAMAGE
#include "x11/extensions/damage.hpp"
#endif
#if WITH_XRENDER
#include "x11/extensions/render.hpp"
#endif
#if WITH_XRANDR
#include "x11/extensions/randr.hpp"
#endif
#if WITH_XSYNC
#include "x11/extensions/sync.hpp"
#endif
#if WITH_XCOMPOSITE
#include "x11/extensions/composite.hpp"
#endif
#if WITH_XKB
#include "x11/extensions/xkb.hpp"
#endif
