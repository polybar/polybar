#pragma once

#include "config.hpp"

#if not ENABLE_SYNC_EXT
#error "X Sync extension is disabled..."
#endif

#include <xcb/sync.h>
#include <xpp/proto/sync.hpp>
