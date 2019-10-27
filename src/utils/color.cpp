#include "utils/color.hpp"

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
 * \returns Empty string for malformed input, either AA for the alpha only
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

  if (hex.length() == 2) {
    // We only have an alpha channel
    return hex;
  }

  if (hex.length() == 3) {
    // RGB -> ARGB
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


rgba::rgba() : m_value(0), m_type(NONE) {}
rgba::rgba(uint32_t value, color_type type) : m_value(value), m_type(type) {}
rgba::rgba(string hex) {
  hex = normalize_hex(hex);

  if (hex.length() == 0) {
    // TODO we need a way to inform the user that their color was malformed
    m_value = 0;
    m_type = NONE;
  } else if (hex.length() == 2) {
    m_value = std::strtoul(hex.c_str(), nullptr, 16) << 24;
    m_type = ALPHA_ONLY;
  } else {
    m_value = std::strtoul(hex.c_str(), nullptr, 16);
    m_type = ARGB;
  }
}

rgba::operator string() const {
  char s[10];
  size_t len = 0;

  len = snprintf(s, 10, "#%08x", m_value);
  return string(s, len);
}

bool rgba::operator==(const rgba& other) const {
  if (m_type != other.m_type) {
    return false;
  }

  switch (m_type) {
    case NONE:
      return true;
    case ARGB:
      return m_value == other.m_value;
    case ALPHA_ONLY:
      return a_int() == other.a_int();
    default:
      return false;
  }
}

rgba::operator uint32_t() const {
  return m_value;
}

double rgba::a() const {
  return a_int() / 255.0;
}

double rgba::r() const {
  return r_int() / 255.0;
}

double rgba::g() const {
  return g_int() / 255.0;
}

double rgba::b() const {
  return b_int() / 255.0;
}

uint8_t rgba::a_int() const {
  return (m_value >> 24) & 0xFF;
}

uint8_t rgba::r_int() const {
  return (m_value >> 16) & 0xFF;
}

uint8_t rgba::g_int() const {
  return (m_value >> 8) & 0xFF;
}

uint8_t rgba::b_int() const {
  return m_value & 0xFF;
}

bool rgba::has_color() const {
  return m_type != NONE;
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
