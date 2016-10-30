#pragma once

#include <iomanip>

#include "common.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

union rgba {
  struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  } _;
  uint32_t v;
};

static map<string, class color> g_colorstore;

class color {
 public:
  explicit color(string hex) : m_hex(string_util::upper(hex)) {
    m_rgba.v = (strtoul(&hex[1], nullptr, 16));
    m_rgb = (m_rgba.v << 8) >> 8;

    // premultiply alpha
    m_rgba._.r = m_rgba._.r * m_rgba._.a / 255;
    m_rgba._.g = m_rgba._.g * m_rgba._.a / 255;
    m_rgba._.b = m_rgba._.b * m_rgba._.a / 255;
  }

  explicit color(uint32_t v) {
    char buffer[7];
    snprintf(buffer, sizeof(buffer), "%06x", v);
    m_hex = "#" + string{buffer};
    m_rgba.v = (strtoul(&m_hex[1], nullptr, 16));
  }

  uint32_t value() const {
    return m_rgba.v;
  }

  uint32_t rgb() const {
    return m_rgb;
  }

  uint32_t alpha() const {
    return 0xFFFF & (((value() >> 24)) | ((value() >> 24)));
  }

  uint32_t red() const {
    return 0xFFFF & (((value() >> 16) << 8) | ((value() >> 16)));
  }

  uint32_t green() const {
    return 0xFFFF & (((value() >> 8) << 8) | ((value() >> 8)));
  }

  uint32_t blue() const {
    return 0xFFFF & ((value() << 8) | value());
  }

  string hex_to_rgb() const {
    // clang-format off
    return string_util::from_stream(stringstream()
        << "#"
        << std::setw(6)
        << std::setfill('0')
        << std::hex
        << std::uppercase
        << (m_rgba.v & 0x00FFFFFF));
    // clang-format on
  }

  string hex_to_rgba() const {
    return m_hex;
  }

  static auto parse(string input, color fallback) {
    auto it = g_colorstore.find(input);
    if (it != g_colorstore.end()) {
      return it->second;
    }
    string hex{input};
    if (hex.substr(0, 1) != "#")
      hex = "#" + hex;
    if (hex.length() == 4)
      hex = {'#', hex[1], hex[1], hex[2], hex[2], hex[3], hex[3]};
    if (hex.length() == 7)
      hex = "#FF" + hex.substr(1);
    if (hex.length() != 9)
      return fallback;
    color result{hex};
    g_colorstore.emplace(input, result);
    return result;
  }

  static auto parse(string input) {
    static color none{0};
    return parse(input, none);
  }

 protected:
  rgba m_rgba;
  unsigned long m_rgb;
  string m_hex;
};

static color g_colorblack{"#ff000000"};
static color g_colorwhite{"#ffffffff"};

LEMONBUDDY_NS_END
