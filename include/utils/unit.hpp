#pragma once

#include <cassert>
#include <cmath>
#include <string>
#include <stdexcept>

#include "components/types.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace unit_utils {
  template <typename ValueType, typename ReturnType = long unsigned int>
  ReturnType point_to_pixel(ValueType point, ValueType dpi) {
    return static_cast<ReturnType>(dpi * point / ValueType(72));
  }

  template <typename ReturnType = int>
  ReturnType spacing_to_pixel(spacing_val size, double dpi) {
    assert(size.type != spacing_type::SPACE);

    if (size.type == spacing_type::PIXEL) {
      return size.value;
    }

    return point_to_pixel<double, ReturnType>(size.value, dpi);
  }

  template <typename ReturnType = int>
  ReturnType extent_to_pixel(const extent_val size, double dpi) {
    if (size.type == extent_type::PIXEL) {
      return size.value;
    }

    return point_to_pixel<double, ReturnType>(size.value, dpi);
  }

  inline extent_val parse_extent(string&& str) {
    size_t pos;
    auto size_value = std::stof(str, &pos);

    extent_type type;

    string unit = string_util::trim(str.substr(pos));
    if (!unit.empty()) {
      if (unit == "px") {
        type = extent_type::PIXEL;
      } else if (unit == "pt") {
        type = extent_type::POINT;
      } else {
        throw std::runtime_error("Unrecognized unit '" + unit + "'");
      }
    } else {
      type = extent_type::PIXEL;
    }

    // Pixel values should be integers
    if (type == extent_type::PIXEL) {
      size_value = std::trunc(size_value);
    }

    return {type, size_value};
  }

  inline string extent_to_string(extent_val extent) {
    if (extent.value > 0) {
      switch (extent.type) {
        case extent_type::POINT:
          return to_string(extent.value) + "pt";
        case extent_type::PIXEL:
          return to_string(static_cast<int>(extent.value)) + "px";
      }
    }

    return {};
  }

} // namespace unit_utils

POLYBAR_NS_END
