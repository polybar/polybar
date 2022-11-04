#include "utils/env.hpp"

#include <cstdlib>
#include <cstring>
#include <utility>

POLYBAR_NS

namespace env_util {
  bool has(const string& var) {
    const char* env{std::getenv(var.c_str())};
    return env != nullptr && std::strlen(env) > 0;
  }

  string get(const string& var, string fallback) {
    const char* value{std::getenv(var.c_str())};
    return value != nullptr ? value : move(fallback);
  }
}  // namespace env_util

POLYBAR_NS_END
