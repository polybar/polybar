#pragma once

#include <string>

#include "components/types.hpp"

POLYBAR_NS

namespace unit_utils {
  template <typename ValueType, typename ReturnType = int>
  ReturnType point_to_pixel(ValueType point, ValueType dpi) {
    return static_cast<ReturnType>(dpi * point / ValueType(72));
  }

  template <typename ReturnType = int>
  ReturnType size_with_unit_to_pixel(size_with_unit size, double dpi) {
    if (size.type == unit_type::PIXEL) {
      return size.value;
    }

    return point_to_pixel<double, ReturnType>(size.value, dpi);
  }

  inline string size_with_unit_to_string(size_with_unit size, double dpi) {
    if (size.value > 0) {
      if (size.type == unit_type::SPACE) {
        return string(static_cast<string::size_type>(size.value), ' ');
      } else {
        if (size.type == unit_type::POINT) {
          size.value = point_to_pixel(static_cast<double>(size.value), dpi);
        }

        return "%{O" + to_string(size.value) + "}";
      }
    }
    return {};
  }
}  // namespace unit_utils

POLYBAR_NS_END
