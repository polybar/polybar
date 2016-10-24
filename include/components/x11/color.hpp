#pragma once

#include <iomanip>

#include "common.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"

union rgba {
  struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  };
  uint32_t v;
};

#pragma clang diagnostic pop

static map<string, class color> g_colorstore;

class color {
 public:
  explicit color(string hex) : m_hex(string_util::upper(hex)) {
    m_rgba.v = static_cast<uint32_t>(strtoul(&hex[1], nullptr, 16));
    // premultiply alpha
    m_rgba.r = m_rgba.r * m_rgba.a / 255;
    m_rgba.g = m_rgba.g * m_rgba.a / 255;
    m_rgba.b = m_rgba.b * m_rgba.a / 255;
  }

  explicit color(uint32_t v) {
    char buffer[7];
    snprintf(buffer, sizeof(buffer), "%06x", v);
    m_hex = "#" + string{buffer};
    m_rgba.v = v;
  }

  uint32_t value() const {
    return m_rgba.v;
  }

  string rgb() const {
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

  string hex() const {
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
  string m_hex;
};

static color g_colorblack{"#ff000000"};
static color g_colorwhite{"#ffffffff"};

LEMONBUDDY_NS_END
