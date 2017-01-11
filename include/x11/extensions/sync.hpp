#pragma once

#include "settings.hpp"

#if not WITH_XSYNC
#error "X Sync extension is disabled..."
#endif

#include <xcb/sync.h>
#include <xpp/proto/sync.hpp>

#include "common.hpp"

POLYBAR_NS

// fwd
class connection;

namespace sync_util {
  void query_extension(connection& conn);
}

POLYBAR_NS_END
