#pragma once

#include "config.hpp"

#if not ENABLE_COMPOSITE_EXT
#error "X Composite extension is disabled..."
#endif

#include <xcb/composite.h>
#include <xpp/proto/composite.hpp>
