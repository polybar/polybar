#pragma once

#include <cstdlib>

#include "common.hpp"

POLYBAR_NS

template <typename T>
using malloc_ptr_t = shared_ptr<T>;

namespace memory_util {
  /**
   * Create a shared pointer using malloc/free
   */
  template <typename T, size_t Size = sizeof(T), typename Deleter = decltype(std::free)>
  inline malloc_ptr_t<T> make_malloc_ptr(Deleter deleter = std::free) {
    return malloc_ptr_t<T>(static_cast<T*>(calloc(1, Size)), deleter);
  }

  /**
   * Get the number of elements in T
   */
  template <typename T>
  inline auto countof(T& p) {
    return sizeof(p) / sizeof(p[0]);
  }
}

POLYBAR_NS_END
