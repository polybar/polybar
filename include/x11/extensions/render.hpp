#pragma once

#include "settings.hpp"

#if not WITH_XRENDER
#error "X Render extension is disabled..."
#endif

#include <xcb/render.h>
#include <xpp/proto/render.hpp>

#include "common.hpp"

POLYBAR_NS

// fwd
class connection;

namespace render_util {
  void query_extension(connection& conn);
}

POLYBAR_NS_END
