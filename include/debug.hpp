#pragma once

#ifndef DEBUG
#error "Not a debug build..."
#endif

#include <chrono>
#include <cstdio>

#include "common.hpp"

POLYBAR_NS

namespace debug_util {
  /**
   * Wrapper that starts tracking the time when created
   * and reports the  duration when it goes out of scope
   */
  class scope_timer {
   public:
    using clock_t = std::chrono::high_resolution_clock;
    using duration_t = std::chrono::milliseconds;

    explicit scope_timer() : m_start(clock_t::now()) {}
    ~scope_timer() {
      printf("%lums\n", std::chrono::duration_cast<duration_t>(clock_t::now() - m_start).count());
    }

   private:
    clock_t::time_point m_start;
  };

  inline unique_ptr<scope_timer> make_scope_timer() {
    return make_unique<scope_timer>();
  }

  template <class T>
  void execution_speed(const T& expr) noexcept {
    auto start = std::chrono::high_resolution_clock::now();
    expr();
    auto finish = std::chrono::high_resolution_clock::now();
    printf("execution speed: %lums\n", std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count());
  }

  template <class T>
  void memory_usage(const T& object) noexcept {
    printf("memory usage: %lub\n", sizeof(object));
  }
}

POLYBAR_NS_END
