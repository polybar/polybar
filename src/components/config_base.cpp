#include "components/config_base.hpp"

#include <chrono>

#include "cairo/utils.hpp"
#include "components/types.hpp"
#include "utils/color.hpp"
#include "utils/factory.hpp"
#include "utils/string.hpp"
#include "utils/units.hpp"

POLYBAR_NS

namespace chrono = std::chrono;
namespace config_utils {

template <>
string convert(string&& value) {
  return forward<string>(value);
}

template <>
const char* convert(string&& value) {
  return value.c_str();
}

template <>
char convert(string&& value) {
  return value.c_str()[0];
}

template <>
int convert(string&& value) {
  return std::strtol(value.c_str(), nullptr, 10);
}

template <>
short convert(string&& value) {
  return static_cast<short>(std::strtol(value.c_str(), nullptr, 10));
}

template <>
bool convert(string&& value) {
  string lower{string_util::lower(forward<string>(value))};

  return (lower == "true" || lower == "yes" || lower == "on" || lower == "1");
}

template <>
float convert(string&& value) {
  return std::strtof(value.c_str(), nullptr);
}

template <>
double convert(string&& value) {
  return std::strtod(value.c_str(), nullptr);
}

template <>
long convert(string&& value) {
  return std::strtol(value.c_str(), nullptr, 10);
}

template <>
long long convert(string&& value) {
  return std::strtoll(value.c_str(), nullptr, 10);
}

template <>
unsigned char convert(string&& value) {
  return std::strtoul(value.c_str(), nullptr, 10);
}

template <>
unsigned short convert(string&& value) {
  return std::strtoul(value.c_str(), nullptr, 10);
}

template <>
unsigned int convert(string&& value) {
  return std::strtoul(value.c_str(), nullptr, 10);
}

template <>
unsigned long convert(string&& value) {
  unsigned long v{std::strtoul(value.c_str(), nullptr, 10)};
  return v < ULONG_MAX ? v : 0UL;
}

template <>
unsigned long long convert(string&& value) {
  unsigned long long v{std::strtoull(value.c_str(), nullptr, 10)};
  return v < ULLONG_MAX ? v : 0ULL;
}

template <>
spacing_val convert(string&& value) {
  return units_utils::parse_spacing(value);
}

template <>
extent_val convert(std::string&& value) {
  return units_utils::parse_extent(value);
}

/**
 * Allows a new format for pixel sizes (like width in the bar section)
 *
 * The new format is X%:Z, where X is in [0, 100], and Z is any real value
 * describing a pixel offset. The actual value is calculated by X% * max + Z
 */
template <>
percentage_with_offset convert(string&& value) {
  size_t i = value.find(':');

  if (i == std::string::npos) {
    if (value.find('%') != std::string::npos) {
      return {std::stod(value), {}};
    } else {
      return {0., convert<extent_val>(move(value))};
    }
  } else {
    std::string percentage = value.substr(0, i - 1);
    return {std::stod(percentage), convert<extent_val>(value.substr(i + 1))};
  }
}

template <>
chrono::seconds convert(string&& value) {
  return chrono::seconds{convert<chrono::seconds::rep>(forward<string>(value))};
}

template <>
chrono::milliseconds convert(string&& value) {
  return chrono::milliseconds{convert<chrono::milliseconds::rep>(forward<string>(value))};
}

template <>
chrono::duration<double> convert(string&& value) {
  return chrono::duration<double>{convert<double>(forward<string>(value))};
}

template <>
rgba convert(string&& value) {
  if (value.empty()) {
    return rgba{};
  }

  rgba ret{value};

  if (!ret.has_color()) {
    throw value_error("\"" + value + "\" is an invalid color value.");
  }

  return ret;
}

template <>
cairo_operator_t convert(string&& value) {
  return cairo::utils::str2operator(forward<string>(value), CAIRO_OPERATOR_OVER);
}

}

POLYBAR_NS_END
