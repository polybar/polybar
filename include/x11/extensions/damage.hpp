#pragma once

#include "settings.hpp"

#if not WITH_XDAMAGE
#error "X Damage extension is disabled..."
#endif

#include <xcb/damage.h>
#include <xpp/proto/damage.hpp>

#include "common.hpp"

POLYBAR_NS

// fwd
class connection;

namespace damage_util {
  void query_extension(connection& conn);
}

POLYBAR_NS_END
