#pragma once

#ifndef DEBUG
#error "Not a debug build..."
#endif

#include <chrono>
#include <iostream>

#include "common.hpp"

POLYBAR_NS

namespace debug_util {
  template <class T>
  void loop(const T& expr, size_t iterations) noexcept {
    while (iterations--) {
      expr();
    }
  }

  template <class T>
  void execution_speed(const T& expr) noexcept {
    auto start = std::chrono::high_resolution_clock::now();
    expr();
    auto finish = std::chrono::high_resolution_clock::now();
    std::cout << "execution speed: " << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count()
              << "ms" << std::endl;
  }

  template <class T>
  void execution_speed(const T& expr, size_t iterations) noexcept {
    execution_speed([=] { loop(expr, iterations); });
  }

  template <class T>
  void memory_usage(const T& object) noexcept {
    std::cout << "memory usage: " << sizeof(object) << "b" << std::endl;
  }
}

POLYBAR_NS_END
