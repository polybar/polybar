#pragma once

#include <string>

#include "components/types.hpp"

POLYBAR_NS

namespace unit_utils {
  template <typename ValueType, typename ReturnType = long unsigned int>
  ReturnType point_to_pixel(ValueType point, ValueType dpi) {
    return static_cast<ReturnType>(dpi * point / ValueType(72));
  }

  template <typename ReturnType = int, typename T>
  ReturnType size_with_unit_to_pixel(details::size_with_unit_impl<T> size, double dpi) {
    if (size.type == unit_type::PIXEL || size.type == unit_type::SPACE) { // For this function space are interpreted as PIXEL
      return size.value;
    }

    return point_to_pixel<double, ReturnType>(size.value, dpi);
  }

  inline string size_with_unit_to_string(size_with_unit size, double dpi) {
    if (size.value > 0) {
      if (size.type == unit_type::SPACE) {
        return string(size.value, ' ');
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
