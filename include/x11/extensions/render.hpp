#pragma once

#include "config.hpp"

#if not WITH_XRENDER
#error "X Render extension is disabled..."
#endif

#include <xcb/render.h>
#include <xpp/proto/render.hpp>
