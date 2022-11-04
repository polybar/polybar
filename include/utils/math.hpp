#pragma once

#include <algorithm>
#include <cmath>

#include "common.hpp"

POLYBAR_NS

namespace math_util {
  /**
   * Limit value T by min and max bounds
   */
  template <typename ValueType>
  ValueType cap(ValueType value, ValueType min_value, ValueType max_value) {
    value = std::min<ValueType>(value, max_value);
    value = std::max<ValueType>(value, min_value);
    return value;
  }

  /**
   * Calculate the percentage for a value
   * between min_value and max_value
   */
  template <typename ValueType, typename ReturnType = int>
  ReturnType unbounded_percentage(ValueType value, ValueType min_value, ValueType max_value) {
    auto upper = (max_value - min_value);
    auto lower = static_cast<float>(value - min_value);
    float percentage = (lower / upper) * 100.0f;
    if (std::is_integral<ReturnType>()) {
      return static_cast<ReturnType>(std::round(percentage));
    } else {
      return percentage;
    }
  }

  /**
   * Calculates percentage for a value and
   * clamps it to  a percentage between 0 and 100
   */
  template <typename ValueType, typename ReturnType = int>
  ReturnType percentage(ValueType value, ValueType min_value, ValueType max_value) {
    auto raw_percentage = unbounded_percentage<ValueType, ReturnType>(value, min_value, max_value);
    return cap<ReturnType>(raw_percentage, 0.0f, 100.0f);
  }

  /**
   * Calculates percentage for a value and
   * clamps it to  a percentage between 0 and 100
   */
  template <typename ValueType, typename ReturnType = int>
  ReturnType percentage(ValueType value, ValueType max_value) {
    return percentage<ValueType, ReturnType>(value, max_value - max_value, max_value);
  }

  /**
   * Get value for signed percentage of `max_value` (cap between -max_value and max_value)
   */
  template <typename ValueType, typename ReturnType = int>
  ReturnType signed_percentage_to_value(ValueType signed_percentage, ValueType max_value) {
    if (std::is_integral<ReturnType>())
      return cap<ReturnType>(signed_percentage * max_value / 100.0f + 0.5f, -max_value, max_value);
    else
      return cap<ReturnType>(signed_percentage * max_value / 100.0f, -max_value, max_value);
  }

  /**
   * Get value for percentage of `max_value` (cap between 0 and max_value)
   */
  template <typename ValueType, typename ReturnType = int>
  ReturnType percentage_to_value(ValueType percentage, ValueType max_value) {
    if (std::is_integral<ReturnType>())
      return cap<ReturnType>(percentage * max_value / 100.0f + 0.5f, 0, max_value);
    else
      return cap<ReturnType>(percentage * max_value / 100.0f, 0.0f, max_value);
  }

  /**
   * Get value for percentage of `min_value` to `max_value`
   */
  template <typename ValueType, typename ReturnType = int>
  ReturnType percentage_to_value(ValueType percentage, ValueType min_value, ValueType max_value) {
    if (std::is_integral<ReturnType>())
      return cap<ReturnType>(percentage * (max_value - min_value) / 100.0f + 0.5f, 0, max_value - min_value) +
             min_value;
    else
      return cap<ReturnType>(percentage * (max_value - min_value) / 100.0f, 0.0f, max_value - min_value) + min_value;
  }

  template <typename ReturnType = int>
  ReturnType nearest_10(double value) {
    return static_cast<ReturnType>(static_cast<int>(value / 10.0 + 0.5) * 10.0);
  }

  template <typename ReturnType = int>
  ReturnType nearest_5(double value) {
    return static_cast<ReturnType>(static_cast<int>(value / 5.0 + 0.5) * 5.0);
  }

  inline int ceil(double value, int step = 1) {
    return static_cast<int>((value * 10 + step * 10 - 1) / (step * 10)) * step;
  }
}  // namespace math_util

POLYBAR_NS_END
