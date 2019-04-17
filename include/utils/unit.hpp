#pragma once

#include <cmath>
#include <string>

#include "components/types.hpp"
#include "utils/string.hpp"

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
  ReturnType geometry_to_pixel(geometry size, double dpi) {
    if (size.type == size_type::PIXEL) {
      return size.value;
    }

    return point_to_pixel<double, ReturnType>(size.value, dpi);
  }

  inline string space_size_to_string(space_size size) {
    if (size.value > 0) {
      switch (size.type) {
        case space_type::SPACE:
          return string(static_cast<string::size_type>(size.value), ' ');
        case space_type::POINT: {
          auto str = to_string(size.value);
          str += "pt";
          return str;
        }
        case space_type::PIXEL: {
          return to_string(static_cast<int>(size.value));
        }
      }
    }

    return {};
  }

  inline geometry geometry_from_string(string&& str) {
    char* new_end;
    auto size_value = std::strtof(str.c_str(), &new_end);

    geometry size{size_type::PIXEL, size_value};

    string unit = string_util::trim(new_end);
    if (!unit.empty()) {
      if (unit == "px") {
        size.value = std::trunc(size.value);
      } else if (unit == "pt") {
        size.type = size_type::POINT;
      }
    }

    return size;
  }

  inline string geometry_to_string(geometry geometry) {
    if (geometry.value > 0) {
      switch (geometry.type) {
        case size_type::POINT: {
          auto str = to_string(geometry.value);
          str += "pt";
          return str;
        }
        case size_type::PIXEL: {
          return to_string(static_cast<int>(geometry.value));
        }
      }
    }

    return {};
  }

}  // namespace unit_utils

POLYBAR_NS_END
