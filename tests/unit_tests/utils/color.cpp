#include "common/test.hpp"
#include "utils/color.hpp"

int main() {
  using namespace polybar;

  "rgb"_test = []{
    uint32_t color{0x123456};
    expect(color_util::alpha_channel<uint8_t>(color) == 0);
    expect(color_util::red_channel<uint8_t>(color) == 0x12);
    expect(color_util::green_channel<uint8_t>(color) == 0x34);
    expect(color_util::green_channel<uint16_t>(color) == 0x3434);
    expect(color_util::blue_channel<uint8_t>(color) == 0x56);
  };

  "rgba"_test = []{
    uint32_t color{0xCC123456};
    expect(color_util::alpha_channel<uint16_t>(color) == 0xCCCC);
    expect(color_util::red_channel<uint16_t>(color) == 0x1212);
    expect(color_util::red_channel<uint8_t>(color) == 0x12);
    expect(color_util::green_channel<uint16_t>(color) == 0x3434);
    expect(color_util::blue_channel<uint16_t>(color) == 0x5656);
  };

  "hex"_test = [] {
    uint32_t colorA{0x123456};
    expect(color_util::hex<uint8_t>(colorA).compare("#123456") == 0);
    uint32_t colorB{0xCC123456};
    expect(color_util::hex<uint16_t>(colorB).compare("#cc123456") == 0);
    uint32_t colorC{0x00ffffff};
    expect(color_util::hex<uint16_t>(colorC).compare("#00ffffff") == 0);
  };

  "simplify"_test = [] {
    expect(color_util::simplify_hex("#ff223344") == "#234");
    expect(color_util::simplify_hex("#ee223344") == "#ee223344");
    expect(color_util::simplify_hex("#ff234567") == "#234567");
  };
}
