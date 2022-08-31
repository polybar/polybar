#pragma once

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>

#include "components/types.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace units_utils {
  int point_to_pixel(double point, double dpi);

  int extent_to_pixel(const extent_val size, double dpi);
  unsigned extent_to_pixel_nonnegative(const extent_val size, double dpi);

  extent_type parse_extent_unit(const string& str);
  extent_val parse_extent(const string& str);

  string extent_to_string(extent_val extent);

  int percentage_with_offset_to_pixel(percentage_with_offset g_format, double max, double dpi);
  unsigned percentage_with_offset_to_pixel_nonnegative(percentage_with_offset g_format, double max, double dpi);

  spacing_type parse_spacing_unit(const string& str);
  spacing_val parse_spacing(const string& str);

  extent_val spacing_to_extent(spacing_val spacing);

} // namespace units_utils

POLYBAR_NS_END
