#pragma once

#include "common.hpp"

POLYBAR_NS

namespace env_util {
  bool has(const string& var);
  string get(const string& var, string fallback = "");
}

POLYBAR_NS_END
