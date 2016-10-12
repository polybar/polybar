#pragma once

#include <algorithm>

#include "common.hpp"

LEMONBUDDY_NS

namespace math_util
{
  /**
   * Limit value T by min and max bounds
   */
  template<typename T>
  T cap(T value, T min_value, T max_value)
  {
    value = std::min<T>(value, max_value);
    value = std::max<T>(value, min_value);
    return value;
  }
}

LEMONBUDDY_NS_END
