#pragma once

#include "config.hpp"

#if not ENABLE_DAMAGE_EXT
#error "X Damage extension is disabled..."
#endif

#include <xcb/damage.h>
#include <xpp/proto/damage.hpp>
