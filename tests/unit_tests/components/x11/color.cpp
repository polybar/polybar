#include <csignal>

#include "components/x11/color.hpp"

int main() {
  using namespace lemonbuddy;

  "color"_test = [] {
    color test{"#33990022"};
    expect(test.hex_to_rgba() == "#33990022");
    expect(test.hex_to_rgb() == "#1E0006");
  };

  "channels"_test = [] {
    color test{"#eefb9281"};
    expect(test.alpha() == 0xee);
    expect(test.red() == 0xfb);
    expect(test.green() == 0x92);
    expect(test.blue() == 0x81);
  };

  "base"_test = [] {
    color test{"#eefb9281"};
    auto hex = test.hex_to_rgb();
    expect(std::strtoul(&hex[0], 0, 16) == 0x000000);
  };

  "cache"_test = [] {
    expect(g_colorstore.size() == size_t{0});
    auto c1 = color::parse("#100");
    expect(g_colorstore.size() == size_t{1});
    auto c2 = color::parse("#200");
    expect(g_colorstore.size() == size_t{2});
    auto c3 = color::parse("#200");
    expect(g_colorstore.size() == size_t{2});
    expect(g_colorstore.find("#100")->second.value() == c1.value());
  };

  "predefined"_test = [] {
    expect(g_colorblack.hex_to_rgba() == "#FF000000");
    expect(g_colorwhite.hex_to_rgba() == "#FFFFFFFF");
  };

  "parse"_test = [] {
    expect(color::parse("#ff9900", g_colorblack).hex_to_rgba() == "#FFFF9900");
    expect(color::parse("invalid", g_colorwhite).hex_to_rgba() == "#FFFFFFFF");
    expect(color::parse("33990022", g_colorwhite).hex_to_rgb() == "#1E0006");
  };
}
