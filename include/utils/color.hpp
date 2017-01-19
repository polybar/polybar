#pragma once

#include "common.hpp"
#include "utils/cache.hpp"

POLYBAR_NS

static cache<string, uint32_t> g_cache_hex;
static cache<uint32_t, string> g_cache_colors;

struct rgba;

namespace color_util {
  template <typename T = uint8_t>
  T alpha_channel(const uint32_t value) {
    uint8_t a = value >> 24;
    if (std::is_same<T, uint8_t>::value)
      return a << 8 / 0xff;
    else if (std::is_same<T, uint16_t>::value)
      return a << 8 | a << 8 / 0xff;
  }

  template <typename T = uint8_t>
  T red_channel(const uint32_t value) {
    uint8_t r = value >> 16;
    if (std::is_same<T, uint8_t>::value)
      return r << 8 / 0xff;
    else if (std::is_same<T, uint16_t>::value)
      return r << 8 | r << 8 / 0xff;
  }

  template <typename T = uint8_t>
  T green_channel(const uint32_t value) {
    uint8_t g = value >> 8;
    if (std::is_same<T, uint8_t>::value)
      return g << 8 / 0xff;
    else if (std::is_same<T, uint16_t>::value)
      return g << 8 | g << 8 / 0xff;
  }

  template <typename T = uint8_t>
  T blue_channel(const uint32_t value) {
    uint8_t b = value;
    if (std::is_same<T, uint8_t>::value)
      return b << 8 / 0xff;
    else if (std::is_same<T, uint16_t>::value)
      return b << 8 | b << 8 / 0xff;
  }

  template <typename T = uint32_t>
  uint32_t premultiply_alpha(const T value) {
    auto a = color_util::alpha_channel(value);
    auto r = color_util::red_channel(value) * a / 255;
    auto g = color_util::green_channel(value) * a / 255;
    auto b = color_util::blue_channel(value) * a / 255;
    return (a << 24) | (r << 16) | (g << 8) | b;
  }

  template <typename T>
  string hex(uint32_t color) {
    string hex;

    if (!g_cache_hex.check(color)) {
      char s[12];
      size_t len = 0;

      uint8_t a = alpha_channel<T>(color);
      uint8_t r = red_channel<T>(color);
      uint8_t g = green_channel<T>(color);
      uint8_t b = blue_channel<T>(color);

      if (std::is_same<T, uint16_t>::value) {
        len = snprintf(s, sizeof(s), "#%02x%02x%02x%02x", a, r, g, b);
      } else if (std::is_same<T, uint8_t>::value) {
        len = snprintf(s, sizeof(s), "#%02x%02x%02x", r, g, b);
      }

      hex = string(s, len);
    }

    return *g_cache_hex.object(color, hex);
  }

  inline string parse_hex(string hex) {
    if (hex[0] != '#')
      hex.insert(0, 1, '#');
    if (hex.length() == 4)
      hex = {'#', hex[1], hex[1], hex[2], hex[2], hex[3], hex[3]};
    if (hex.length() == 7)
      hex = "#ff" + hex.substr(1);
    if (hex.length() != 9)
      return "";
    return hex;
  }

  inline uint32_t parse(string hex, uint32_t fallback = 0) {
    if ((hex = parse_hex(hex)).empty())
      return fallback;
    return strtoul(&hex[1], nullptr, 16);
  }

  inline string simplify_hex(string hex) {
    // convert #ffrrggbb to #rrggbb
    if (hex.length() == 9 && hex[1] == 'f' && hex[2] == 'f') {
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
}

struct rgb {
  double r;
  double g;
  double b;

  // clang-format off
  explicit rgb(double r, double g, double b) : r(r), g(g), b(b) {}
  explicit rgb(uint32_t color) : rgb(
      color_util::red_channel<uint8_t>(color_util::premultiply_alpha(color)   / 255.0),
      color_util::green_channel<uint8_t>(color_util::premultiply_alpha(color) / 255.0),
      color_util::blue_channel<uint8_t>(color_util::premultiply_alpha(color)  / 255.0)) {}
  // clang-format on
};

struct rgba {
  double r;
  double g;
  double b;
  double a;

  // clang-format off
  explicit rgba(double r, double g, double b, double a) : r(r), g(g), b(b), a(a) {}
  explicit rgba(uint32_t color) : rgba(
      color_util::red_channel<uint8_t>(color)   / 255.0,
      color_util::green_channel<uint8_t>(color) / 255.0,
      color_util::blue_channel<uint8_t>(color)  / 255.0,
      color_util::alpha_channel<uint8_t>(color) / 255.0) {}
  // clang-format on

  operator uint32_t() {
    // clang-format off
    return static_cast<int>(a * 255) << 24
         | static_cast<int>(r * 255) << 16
         | static_cast<int>(g * 255) << 8
         | static_cast<int>(b * 255);
    // clang-format on
  }
};

POLYBAR_NS_END
