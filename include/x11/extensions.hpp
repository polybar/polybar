#pragma once

#include "config.hpp"

#if WITH_XDAMAGE
#include "x11/damage.hpp"
#endif
#if WITH_XRENDER
#include "x11/render.hpp"
#endif
#if WITH_XRANDR
#include "x11/randr.hpp"
#endif
#if WITH_XSYNC
#include "x11/sync.hpp"
#endif
#if WITH_XCOMPOSITE
#include "x11/composite.hpp"
#endif
#if WITH_XKB
#include "x11/xkb.hpp"
#endif
