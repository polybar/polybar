#include "utils/units.hpp"

#include "common.hpp"
#include "components/types.hpp"
#include "utils/math.hpp"

POLYBAR_NS

namespace units_utils {

  int point_to_pixel(double point, double dpi) {
    return dpi * point / 72.0;
  }

  int extent_to_pixel(const extent_val size, double dpi) {
    if (size.type == extent_type::PIXEL) {
      return size.value;
    }

    return point_to_pixel(size.value, dpi);
  }

  /**
   * Converts a percentage with offset into pixels
   */
  unsigned int percentage_with_offset_to_pixel(percentage_with_offset g_format, double max, double dpi) {
    auto offset_pixel = extent_to_pixel(g_format.offset, dpi);

    return static_cast<unsigned int>(math_util::max<double>(
        0, math_util::percentage_to_value<double, double>(g_format.percentage, max) + offset_pixel));
  }

  extent_val parse_extent(string&& str) {
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

  string extent_to_string(extent_val extent) {
    switch (extent.type) {
      case extent_type::POINT:
        return to_string(extent.value) + "pt";
      case extent_type::PIXEL:
        return to_string(static_cast<int>(extent.value)) + "px";
    }

    return {};
  }

} // namespace units_utils

POLYBAR_NS_END
