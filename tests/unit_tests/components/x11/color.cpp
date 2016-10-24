#include <csignal>

#include "components/x11/color.hpp"

int main() {
  using namespace lemonbuddy;

  "color"_test = [] {
    color test{"#33990022"};
    expect(test.hex() == "#33990022");
    expect(test.rgb() == "#1E0006");
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
    expect(g_colorblack.hex() == "#FF000000");
    expect(g_colorwhite.hex() == "#FFFFFFFF");
  };

  "parse"_test = [] {
    expect(color::parse("#ff9900", g_colorblack).hex() == "#FFFF9900");
    expect(color::parse("invalid", g_colorwhite).hex() == "#FFFFFFFF");
    expect(color::parse("33990022", g_colorwhite).rgb() == "#1E0006");
  };
}
