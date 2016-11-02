#include <iomanip>

#include "utils/color.hpp"
#include "utils/string.hpp"
#include "x11/color.hpp"

LEMONBUDDY_NS

map<string, color> g_colorstore;
color g_colorempty{"#00000000"};
color g_colorblack{"#ff000000"};
color g_colorwhite{"#ffffffff"};

color::color(string hex) : m_source(hex) {
  if (hex.empty()) {
    throw application_error("Cannot create color from empty hex");
  }

  uint32_t value = std::strtoul(&hex[1], nullptr, 16);

  // Premultiply alpha
  auto a = color_util::alpha_channel(value);
  auto r = color_util::red_channel(value) * a / 255;
  auto g = color_util::green_channel(value) * a / 255;
  auto b = color_util::blue_channel(value) * a / 255;

  m_color = (a << 24) | (r << 16) | (g << 8) | b;
}

string color::source() const {
  return m_source;
}

color::operator XRenderColor() const {
  XRenderColor x;
  x.red = color_util::red_channel<uint16_t>(m_color);
  x.green = color_util::green_channel<uint16_t>(m_color);
  x.blue = color_util::blue_channel<uint16_t>(m_color);
  x.alpha = color_util::alpha_channel<uint16_t>(m_color);
  return x;
}

color::operator string() const {
  return color_util::hex(m_color);
}

color::operator uint32_t() const {
  return m_color;
}

color color::parse(string input, color fallback) {
  if (input.empty()) {
    throw application_error("Cannot parse empty color");
  }

  auto it = g_colorstore.find(input);

  if (it != g_colorstore.end()) {
    return it->second;
  }

  if ((input = color_util::parse_hex(input)).empty()) {
    return fallback;
  }

  color result{input};
  g_colorstore.emplace_hint(it, input, result);

  return result;
}

color color::parse(string input) {
  return parse(input, g_colorempty);
}

LEMONBUDDY_NS_END
