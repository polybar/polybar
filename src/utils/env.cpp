#include <cstring>
#include <cstdlib>
#include <thread>
#include <utility>

#include "utils/env.hpp"

POLYBAR_NS

namespace env_util {
  bool has(const char* var) {
    const char* env{std::getenv(var)};
    return env != nullptr && std::strlen(env) > 0;
  }

  string get(const char* var, string fallback) {
    const char* value{std::getenv(var)};
    return value != nullptr ? value : move(fallback);
  }
}

POLYBAR_NS_END
