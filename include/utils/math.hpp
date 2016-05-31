#pragma once

#include <string>
#include <algorithm>

namespace math
{
  template<typename T>
  T cap(T value, T min_value, T max_value)
  {
    value = std::min<T>(value, max_value);
    value = std::max<T>(value, min_value);
    return value;
  }
}
