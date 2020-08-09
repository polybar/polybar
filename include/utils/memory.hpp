#pragma once

#include <cstdlib>

#include "common.hpp"

POLYBAR_NS

template <typename T>
using malloc_ptr_t = shared_ptr<T>;

namespace memory_util {
  /**
   * Create a shared pointer using malloc/free
   *
   * Generates a noexcept-type warning because the mangled name for
   * malloc_ptr_t will change in C++17. This doesn't affect use because we have
   * no public ABI, so we ignore it here.
   * See also this SO answer: https://stackoverflow.com/a/46857525/5363071
   */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnoexcept-type"
  template <typename T, size_t Size = sizeof(T), typename Deleter = decltype(std::free)>
  inline malloc_ptr_t<T> make_malloc_ptr(Deleter deleter = std::free) {
    return malloc_ptr_t<T>(static_cast<T*>(calloc(1, Size)), deleter);
  }
#pragma GCC diagnostic pop

  /**
   * Get the number of elements in T
   */
  template <typename T>
  inline auto countof(T& p) {
    return sizeof(p) / sizeof(p[0]);
  }
}  // namespace memory_util

POLYBAR_NS_END
