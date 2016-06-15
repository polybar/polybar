#ifdef DEBUG
#pragma once

#include <iostream>

template <class T>
void benchmark_execution_speed(const T& expr) noexcept {
  auto start = std::chrono::high_resolution_clock::now();
  expr();
  auto finish = std::chrono::high_resolution_clock::now();
  std::cout << "execution speed: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << "ms"
            << std::endl;
}

template <class T>
void benchmark_memory_usage(const T& object) noexcept {
  std::cout << "memory usage: " << sizeof(object) << "b" << std::endl;
}

#endif
