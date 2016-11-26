#pragma once

#include "config.hpp"

#if not ENABLE_RENDER_EXT
#error "X Render extension is disabled..."
#endif

#include <xcb/render.h>
#include <xpp/proto/render.hpp>
