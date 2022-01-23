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

  extent_val parse_extent(string&& str);

  string extent_to_string(extent_val extent);

  unsigned percentage_with_offset_to_pixel(percentage_with_offset g_format, double max, double dpi);

} // namespace units_utils

POLYBAR_NS_END
