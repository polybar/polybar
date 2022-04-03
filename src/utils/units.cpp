#include "utils/units.hpp"

#include "common.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "utils/math.hpp"

POLYBAR_NS

namespace units_utils {

  /**
   * Converts points to pixels under the given DPI (PPI).
   *
   * 1 pt = 1/72in, so point / 72 * DPI = #pixels
   */
  int point_to_pixel(double point, double dpi) {
    return dpi * point / 72.0;
  }

  /**
   * Converts an extent value to a pixel value according to the given DPI (if needed).
   */
  int extent_to_pixel(const extent_val size, double dpi) {
    if (size.type == extent_type::PIXEL) {
      return size.value;
    }

    return point_to_pixel(size.value, dpi);
  }

  /**
   * Same as extent_to_pixel but is capped below at 0 pixels.
   */
  unsigned extent_to_pixel_nonnegative(const extent_val size, double dpi) {
    return std::max(0, extent_to_pixel(size, dpi));
  }

  /**
   * Converts a percentage with offset into pixels
   */
  int percentage_with_offset_to_pixel(percentage_with_offset g_format, double max, double dpi) {
    int offset_pixel = extent_to_pixel(g_format.offset, dpi);

    return static_cast<int>(math_util::percentage_to_value<double, double>(g_format.percentage, max) + offset_pixel);
  }

  /**
   * Same as percentage_with_offset_to_pixel but capped below at 0 pixels
   */
  unsigned percentage_with_offset_to_pixel_nonnegative(percentage_with_offset g_format, double max, double dpi) {
    return std::max<int>(0, percentage_with_offset_to_pixel(g_format, max, dpi));
  }

  extent_type parse_extent_unit(const string& str) {
    if (!str.empty()) {
      if (str == "px") {
        return extent_type::PIXEL;
      } else if (str == "pt") {
        return extent_type::POINT;
      } else {
        throw std::runtime_error("Unrecognized unit '" + str + "'");
      }
    } else {
      return extent_type::PIXEL;
    }
  }

  extent_val parse_extent(const string& str) {
    size_t pos;
    auto size_value = std::stof(str, &pos);

    string unit = string_util::trim(str.substr(pos));
    extent_type type = parse_extent_unit(unit);

    // Pixel values should be integers
    if (type == extent_type::PIXEL) {
      size_value = std::trunc(size_value);
    }

    return {type, size_value};
  }

  string extent_to_string(extent_val extent) {
    std::stringstream ss;

    switch (extent.type) {
      case extent_type::POINT:
        ss << extent.value << "pt";
        break;
      case extent_type::PIXEL:
        ss << static_cast<int>(extent.value) << "px";
        break;
    }

    return ss.str();
  }

  spacing_type parse_spacing_unit(const string& str) {
    if (!str.empty()) {
      if (str == "px") {
        return spacing_type::PIXEL;
      } else if (str == "pt") {
        return spacing_type::POINT;
      } else {
        throw std::runtime_error("Unrecognized unit '" + str + "'");
      }
    } else {
      return spacing_type::SPACE;
    }
  }

  spacing_val parse_spacing(const string& str) {
    size_t pos;
    auto size_value = std::stof(str, &pos);

    if (size_value < 0) {
      throw runtime_error(sstream() << "value '" << str << "' must not be negative");
    }

    spacing_type type;

    string unit = string_util::trim(str.substr(pos));
    if (!unit.empty()) {
      if (unit == "px") {
        type = spacing_type::PIXEL;
        size_value = std::trunc(size_value);
      } else if (unit == "pt") {
        type = spacing_type::POINT;
      } else {
        throw runtime_error("Unrecognized unit '" + unit + "'");
      }
    } else {
      type = spacing_type::SPACE;
      size_value = std::trunc(size_value);
    }

    return {type, size_value};
  }

  /**
   * Creates an extent from a spacing value.
   *
   * @param spacing Value to convert, must not be SPACE
   */
  extent_val spacing_to_extent(spacing_val spacing) {
    extent_type t;

    switch (spacing.type) {
      case spacing_type::POINT:
        t = extent_type::POINT;
        break;
      case spacing_type::PIXEL:
        t = extent_type::PIXEL;
        break;
      default:
        throw std::runtime_error("spacing_to_extent: Illegal type: " + to_string(to_integral(spacing.type)));
    }

    return {t, spacing.value};
  }

} // namespace units_utils

POLYBAR_NS_END
