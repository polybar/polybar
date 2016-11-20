#pragma once

#include "common.hpp"

POLYBAR_NS

namespace env_util {
  bool has(const char* var);
  string get(const char* var, string fallback = "");
}

POLYBAR_NS_END
