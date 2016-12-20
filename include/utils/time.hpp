#pragma once

#include <chrono>

#include "common.hpp"

POLYBAR_NS

namespace chrono = std::chrono;

namespace time_util {
  using clock_t = chrono::high_resolution_clock;

  template <typename Duration = chrono::milliseconds, typename T>
  auto measure(const T& expr) noexcept {
    auto start = clock_t::now();
    expr();
    auto finish = clock_t::now();
    return chrono::duration_cast<Duration>(finish - start).count();
  }
}

POLYBAR_NS_END
