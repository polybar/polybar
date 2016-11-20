#include <thread>

#include "utils/env.hpp"

POLYBAR_NS

namespace env_util {
  bool has(const char* var) {
    return std::getenv(var) != nullptr;
  }

  string get(const char* var, string fallback) {
    const char* value{std::getenv(var)};
    return value != nullptr ? value : fallback;
  }
}

POLYBAR_NS_END
