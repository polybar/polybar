#pragma once

#include "config.hpp"

#if ENABLE_DAMAGE_EXT
#include "x11/damage.hpp"
#endif
#if ENABLE_RENDER_EXT
#include "x11/render.hpp"
#endif
#if ENABLE_RANDR_EXT
#include "x11/randr.hpp"
#endif
#if ENABLE_SYNC_EXT
#include "x11/sync.hpp"
#endif
#if ENABLE_COMPOSITE_EXT
#include "x11/composite.hpp"
#endif
#if ENABLE_XKB_EXT
#include "x11/xkb.hpp"
#endif
