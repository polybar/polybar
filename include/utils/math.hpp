#pragma once

#include <algorithm>

#include "common.hpp"

LEMONBUDDY_NS

namespace math_util {
  /**
   * Limit value T by min and max bounds
   */
  template <typename T>
  T cap(T value, T min_value, T max_value) {
    value = std::min<T>(value, max_value);
    value = std::max<T>(value, min_value);
    return value;
  }

  /**
   * Calculate the percentage for a value
   * within given range
   */
  template <typename T>
  int percentage(T value, T min_value, T max_value) {
    T percentage = ((value - min_value) / (max_value - min_value)) * 100.0f + 0.5f;
    return cap<T>(std::ceil(percentage), 0, 100);
  }
}

LEMONBUDDY_NS_END
