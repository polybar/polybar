#pragma once

#include <cstring>

#include "common.hpp"

POLYBAR_NS

namespace memory_util {
  /**
   * Create a shared pointer using malloc/free
   */
  template <typename T, typename Deleter = decltype(free)>
  inline auto make_malloc_ptr(size_t size = sizeof(T), Deleter deleter = free) {
    shared_ptr<T> ptr{static_cast<T*>(malloc(size)), deleter};
    memset(ptr.get(), 0, size);
    return ptr;
  }

  /**
   * Get the number of elements in T
   */
  template <typename T>
  inline auto countof(T& p) {
    return sizeof(p) / sizeof(p[0]);
  }

  template <typename T>
  using malloc_ptr_t = shared_ptr<T>;
}

POLYBAR_NS_END
