#pragma once

#include "settings.hpp"

#if not WITH_XCOMPOSITE
#error "X Composite extension is disabled..."
#endif

#include <xcb/composite.h>
#include <xpp/proto/composite.hpp>

#include "common.hpp"

POLYBAR_NS

// fwd
class connection;

namespace composite_util {
  void query_extension(connection& conn);
}

POLYBAR_NS_END
