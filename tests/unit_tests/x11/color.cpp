#include <csignal>

#include "utils/color.hpp"
#include "x11/color.cpp"

int main() {
  using namespace polybar;

  "color"_test = [] {
    color test{"#33990022"};
    expect(color_util::hex<uint8_t>(test) == "#1E0006");
    expect(color_util::hex<uint16_t>(test) == "#33990022");
  };

  "channels"_test = [] {
    color test{"#eefb9281"};
    expect(color_util::alpha_channel<uint8_t>(test) == 0xee);
    expect(color_util::red_channel<uint8_t>(test) == 0xfb);
    expect(color_util::green_channel<uint8_t>(test) == 0x92);
    expect(color_util::blue_channel<uint8_t>(test) == 0x81);
  };

  "base"_test = [] {
    color test{"#eefb9281"};
    auto hex = color_util::hex<uint8_t>(test);
    expect(std::strtoul(&hex[0], 0, 16) == 0x0);
  };

  "cache"_test = [] {
    expect(g_colorstore.size() == size_t{0});
    auto c1 = color::parse("#100");
    expect(g_colorstore.size() == size_t{1});
    auto c2 = color::parse("#200");
    expect(g_colorstore.size() == size_t{2});
    auto c3 = color::parse("#200");
    expect(g_colorstore.size() == size_t{2});
    expect((uint32_t)g_colorstore.find("#100")->second == (uint32_t)c1);
  };

  "predefined"_test = [] {
    expect(color_util::hex<uint16_t>(g_colorblack) == "#FF000000");
    expect(color_util::hex<uint16_t>(g_colorwhite) == "#FFFFFFFF");
  };

  "parse"_test = [] {
    expect(color_util::hex<uint16_t>(color::parse("#ff9900", g_colorblack)) == "#FFFF9900");
    expect(color_util::hex<uint16_t>(color::parse("invalid", g_colorwhite)) == "#FFFFFFFF");
    expect(color_util::hex<uint8_t>(color::parse("33990022", g_colorwhite)) == "#1E0006");
  };
}
