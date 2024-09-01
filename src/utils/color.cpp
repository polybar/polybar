#include "utils/color.hpp"

#include <algorithm>
#include <cstdint>

POLYBAR_NS

/**
 * Takes a hex string as input and brings it into a normalized form
 *
 * The input can either be only an alpha channel #AA
 * or any of these forms: #RGB, #ARGB, #RRGGBB, #AARRGGBB
 *
 * Colors without alpha channel will get an alpha channel of FF
 * The input does not have to start with '#'
 *
 * @returns Empty string for malformed input, either AA for the alpha only
 * input or an 8 character string of the expanded form AARRGGBB
 */
static string normalize_hex(string hex) {
  if (hex.length() == 0) {
    return "";
  }

  // We remove the hash because it makes processing easier
  if (hex[0] == '#') {
    hex.erase(0, 1);
  }

  // Check that only valid characters are used
  if (!std::all_of(hex.cbegin(), hex.cend(), isxdigit)) {
    return "";
  }

  if (hex.length() == 2) {
    // We only have an alpha channel
    return hex;
  }

  if (hex.length() == 3) {
    // RGB -> FRGB
    hex.insert(0, 1, 'f');
  }

  if (hex.length() == 4) {
    // ARGB -> AARRGGBB
    hex = {hex[0], hex[0], hex[1], hex[1], hex[2], hex[2], hex[3], hex[3]};
  }

  if (hex.length() == 6) {
    // RRGGBB -> FFRRGGBB
    hex.insert(0, 2, 'f');
  }

  if (hex.length() != 8) {
    return "";
  }

  return hex;
}

rgba::rgba() : m_value(0), m_type(type::NONE) {}
rgba::rgba(uint32_t value, enum type type) : m_value(value), m_type(type) {}
rgba::rgba(string hex) {
  hex = normalize_hex(hex);

  if (hex.length() == 0) {
    m_value = 0;
    m_type = type::NONE;
  } else if (hex.length() == 2) {
    m_value = std::strtoul(hex.c_str(), nullptr, 16) << 24;
    m_type = type::ALPHA_ONLY;
  } else {
    m_value = std::strtoul(hex.c_str(), nullptr, 16);
    m_type = type::ARGB;
  }
}

rgba::operator string() const {
  char s[10];
  size_t len = snprintf(s, 10, "#%08x", m_value);
  return string(s, len);
}

bool rgba::operator==(const rgba& other) const {
  if (m_type != other.m_type) {
    return false;
  }

  switch (m_type) {
    case type::NONE:
      return true;
    case type::ARGB:
      return m_value == other.m_value;
    case type::ALPHA_ONLY:
      return alpha_i() == other.alpha_i();
    default:
      return false;
  }
}

bool rgba::operator!=(const rgba& other) const {
  return !(*this == other);
}

rgba::operator uint32_t() const {
  return m_value;
}

rgba::operator bool() const {
  return has_color();
}

uint32_t rgba::value() const {
  return this->m_value;
}

enum rgba::type rgba::get_type() const {
  return m_type;
}

double rgba::alpha_d() const {
  return alpha_i() / 255.0;
}

double rgba::red_d() const {
  return red_i() / 255.0;
}

double rgba::green_d() const {
  return green_i() / 255.0;
}

double rgba::blue_d() const {
  return blue_i() / 255.0;
}

uint8_t rgba::alpha_i() const {
  return (m_value >> 24) & 0xFF;
}

uint8_t rgba::red_i() const {
  return (m_value >> 16) & 0xFF;
}

uint8_t rgba::green_i() const {
  return (m_value >> 8) & 0xFF;
}

uint8_t rgba::blue_i() const {
  return m_value & 0xFF;
}

bool rgba::has_color() const {
  return m_type != type::NONE;
}

bool rgba::is_transparent() const {
  return alpha_i() != 0xFF;
}

/**
 * Applies the alpha channel of this color to the given color.
 */
rgba rgba::apply_alpha_to(rgba other) const {
  uint32_t val = (other.value() & 0x00FFFFFF) | (((uint32_t)alpha_i()) << 24);
  return rgba(val, m_type);
}

/**
 * If this is an ALPHA_ONLY color, applies this alpha channel to the other
 * color, otherwise just returns this.
 *
 * @returns the new color if this is ALPHA_ONLY or a copy of this otherwise.
 */
rgba rgba::try_apply_alpha_to(rgba other) const {
  if (m_type == type::ALPHA_ONLY) {
    return apply_alpha_to(other);
  }

  return *this;
}

string color_util::simplify_hex(string hex) {
  // convert #ffrrggbb to #rrggbb
  if (hex.length() == 9 && std::toupper(hex[1]) == 'F' && std::toupper(hex[2]) == 'F') {
    hex.erase(1, 2);
  }

  // convert #rrggbb to #rgb
  if (hex.length() == 7) {
    if (hex[1] == hex[2] && hex[3] == hex[4] && hex[5] == hex[6]) {
      hex = {'#', hex[1], hex[3], hex[5]};
    }
  }

  return hex;
}

POLYBAR_NS_END
