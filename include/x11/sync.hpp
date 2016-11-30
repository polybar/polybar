#pragma once

#include "config.hpp"

#if not WITH_XSYNC
#error "X Sync extension is disabled..."
#endif

#include <xcb/sync.h>
#include <xpp/proto/sync.hpp>
