#include "common/test.hpp"
#include "utils/color.hpp"

int main() {
  using namespace lemonbuddy;

  "rgb"_test = []{
    auto color = color_util::make_24bit(0x123456);
    expect(color_util::alpha(color) == 0);
    expect(color_util::red<uint8_t>(color) == 0x12);
    expect(color_util::green<uint8_t>(color) == 0x34);
    expect(color_util::blue<uint8_t>(color) == 0x56);
  };

  "rgba"_test = []{
    auto color = color_util::make_32bit(0xCC123456);
    expect(color_util::alpha(color) == 0xCC);
    expect(color_util::red<uint16_t>(color) == 0x1212);
    expect(color_util::green<uint16_t>(color) == 0x3434);
    expect(color_util::blue<uint16_t>(color) == 0x5656);
  };

  "hex"_test = [] {
    auto colorA = color_util::make_24bit(0x123456);
    expect(color_util::hex(colorA) == "#123456");
    auto colorB = color_util::make_32bit(0xCC123456);
    expect(color_util::hex(colorB) == "#CC123456");
  };
}
