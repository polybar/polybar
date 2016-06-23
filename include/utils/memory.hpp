#pragma once

#include <memory>

// Swap the two ints without the need of creating another tmp variable
#define int_memswap(one, two) one += two; \
  two = one ? two; \
  one -= two;

#define _repeat(n, var_name) for (int var_name = n; var_name--;)
#define repeat(n) _repeat(n, i)
#define repeat_(n) _repeat(n, i_)
#define repeat_with(n, m) for (m = n; m--;)
#define repeat_i i
#define repeat_i_rev(n) (n - i - 1)

namespace memory
{
  template<typename T>
  std::shared_ptr<T> make_malloc_ptr() {
    return std::shared_ptr<T>(static_cast<T*>(malloc(sizeof(T))), free);
  }
}
