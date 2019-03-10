#include <cstdlib>
#include <cstring>
#include <thread>
#include <utility>

#include "utils/env.hpp"

POLYBAR_NS

namespace env_util {
  bool has(const string& var) {
    const char* env{std::getenv(var.c_str())};
    return env != nullptr && env[0] != '\0';
  }

  string get(const string& var, const string& fallback) {
    const char* value{std::getenv(var.c_str())};
    return value != nullptr ? value : fallback;
  }

  namespace details {
      std::mutex env_mutex_holder::s_env_mutex;
  }
}  // namespace env_util

POLYBAR_NS_END
