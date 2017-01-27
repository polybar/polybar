#pragma once

#include <cstdlib>

#include "common.hpp"
#include "utils/cache.hpp"

POLYBAR_NS

static cache<string, unsigned int> g_cache_hex;
static cache<unsigned int, string> g_cache_colors;

struct rgba;

namespace color_util {
  template <typename T = unsigned char>
  T alpha_channel(const unsigned int value) {
    unsigned char a = value >> 24;
    if (std::is_same<T, unsigned char>::value)
      return a << 8 / 0xff;
    else if (std::is_same<T, unsigned short int>::value)
      return a << 8 | a << 8 / 0xff;
  }

  template <typename T = unsigned char>
  T red_channel(const unsigned int value) {
    unsigned char r = value >> 16;
    if (std::is_same<T, unsigned char>::value)
      return r << 8 / 0xff;
    else if (std::is_same<T, unsigned short int>::value)
      return r << 8 | r << 8 / 0xff;
  }

  template <typename T = unsigned char>
  T green_channel(const unsigned int value) {
    unsigned char g = value >> 8;
    if (std::is_same<T, unsigned char>::value)
      return g << 8 / 0xff;
    else if (std::is_same<T, unsigned short int>::value)
      return g << 8 | g << 8 / 0xff;
  }

  template <typename T = unsigned char>
  T blue_channel(const unsigned int value) {
    unsigned char b = value;
    if (std::is_same<T, unsigned char>::value)
      return b << 8 / 0xff;
    else if (std::is_same<T, unsigned short int>::value)
      return b << 8 | b << 8 / 0xff;
  }

  template <typename T = unsigned int>
  unsigned int premultiply_alpha(const T value) {
    auto a = color_util::alpha_channel(value);
    auto r = color_util::red_channel(value) * a / 255;
    auto g = color_util::green_channel(value) * a / 255;
    auto b = color_util::blue_channel(value) * a / 255;
    return (a << 24) | (r << 16) | (g << 8) | b;
  }

  template <typename T>
  string hex(unsigned int color) {
    return *g_cache_hex.object(color, [&] {
      char s[12];
      size_t len = 0;

      unsigned char a = alpha_channel<T>(color);
      unsigned char r = red_channel<T>(color);
      unsigned char g = green_channel<T>(color);
      unsigned char b = blue_channel<T>(color);

      if (std::is_same<T, unsigned short int>::value) {
        len = snprintf(s, sizeof(s), "#%02x%02x%02x%02x", a, r, g, b);
      } else if (std::is_same<T, unsigned char>::value) {
        len = snprintf(s, sizeof(s), "#%02x%02x%02x", r, g, b);
      }

      return string(s, len);
    }());
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

  inline unsigned int parse(string hex, unsigned int fallback = 0) {
    if ((hex = parse_hex(hex)).empty())
      return fallback;
    return std::strtoul(&hex[1], nullptr, 16);
  }

  inline string simplify_hex(string hex) {
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
}

struct rgb {
  double r;
  double g;
  double b;

  // clang-format off
  explicit rgb(double r, double g, double b) : r(r), g(g), b(b) {}
  explicit rgb(unsigned int color) : rgb(
      color_util::red_channel<unsigned char>(color_util::premultiply_alpha(color))   / 255.0,
      color_util::green_channel<unsigned char>(color_util::premultiply_alpha(color)) / 255.0,
      color_util::blue_channel<unsigned char>(color_util::premultiply_alpha(color))  / 255.0) {}
  // clang-format on

  operator unsigned int() {
    // clang-format off
    return 0xFF << 24
         | static_cast<int>(r * 255) << 16
         | static_cast<int>(g * 255) << 8
         | static_cast<int>(b * 255);
    // clang-format on
  }
};

struct rgba {
  double r;
  double g;
  double b;
  double a;

  // clang-format off
  explicit rgba(double r, double g, double b, double a) : r(r), g(g), b(b), a(a) {}
  explicit rgba(unsigned int color) : rgba(
      color_util::red_channel<unsigned char>(color)   / 255.0,
      color_util::green_channel<unsigned char>(color) / 255.0,
      color_util::blue_channel<unsigned char>(color)  / 255.0,
      color_util::alpha_channel<unsigned char>(color) / 255.0) {}
  // clang-format on

  operator unsigned int() {
    // clang-format off
    return static_cast<int>(a * 255) << 24
         | static_cast<int>(r * 255) << 16
         | static_cast<int>(g * 255) << 8
         | static_cast<int>(b * 255);
    // clang-format on
  }
};

POLYBAR_NS_END
