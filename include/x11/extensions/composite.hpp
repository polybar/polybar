#pragma once

#include "settings.hpp"

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
