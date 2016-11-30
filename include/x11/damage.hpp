#pragma once

#include "config.hpp"

#if not WITH_XDAMAGE
#error "X Damage extension is disabled..."
#endif

#include <xcb/damage.h>
#include <xpp/proto/damage.hpp>
