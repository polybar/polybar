#pragma once

#include <string>

#include "components/types.hpp"

POLYBAR_NS

namespace unit_utils {
  template <typename ValueType, typename ReturnType = long unsigned int>
  ReturnType point_to_pixel(ValueType point, ValueType dpi) {
    return static_cast<ReturnType>(dpi * point / ValueType(72));
  }

  template <typename ReturnType = int>
  ReturnType space_type_to_pixel(space_size size, double dpi) {
    assert(size.type != space_type::SPACE);

    if (size.type == space_type::PIXEL) {
      return size.value;
    }

    return point_to_pixel<double, ReturnType>(size.value, dpi);
  }

  template <typename ReturnType = int>
  ReturnType geometry_type_to_pixel(ssize_with_unit size, double dpi) {
    if (size.type == size_type::PIXEL) {
      return size.value;
    }

    return point_to_pixel<double, ReturnType>(size.value, dpi);
  }

  inline string size_with_unit_to_string(space_size size, double dpi) {
    if (size.value > 0) {
      if (size.type == space_type::SPACE) {
        return string(static_cast<string::size_type>(size.value), ' ');
      } else {
        if (size.type == space_type::POINT) {
          size.value = point_to_pixel(static_cast<double>(size.value), dpi);
        }

        return "%{O" + to_string(size.value) + "}";
      }
    }
    return {};
  }
}  // namespace unit_utils

POLYBAR_NS_END
