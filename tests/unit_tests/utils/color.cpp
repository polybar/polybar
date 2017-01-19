#include "common/test.hpp"
#include "utils/color.hpp"

int main() {
  using namespace polybar;

  "rgb"_test = []{
    unsigned int color{0x123456};
    expect(color_util::alpha_channel<unsigned char>(color) == 0);
    expect(color_util::red_channel<unsigned char>(color) == 0x12);
    expect(color_util::green_channel<unsigned char>(color) == 0x34);
    expect(color_util::green_channel<unsigned short int>(color) == 0x3434);
    expect(color_util::blue_channel<unsigned char>(color) == 0x56);
  };

  "rgba"_test = []{
    unsigned int color{0xCC123456};
    expect(color_util::alpha_channel<unsigned short int>(color) == 0xCCCC);
    expect(color_util::red_channel<unsigned short int>(color) == 0x1212);
    expect(color_util::red_channel<unsigned char>(color) == 0x12);
    expect(color_util::green_channel<unsigned short int>(color) == 0x3434);
    expect(color_util::blue_channel<unsigned short int>(color) == 0x5656);
  };

  "hex"_test = [] {
    unsigned int colorA{0x123456};
    expect(color_util::hex<unsigned char>(colorA).compare("#123456") == 0);
    unsigned int colorB{0xCC123456};
    expect(color_util::hex<unsigned short int>(colorB).compare("#cc123456") == 0);
    unsigned int colorC{0x00ffffff};
    expect(color_util::hex<unsigned short int>(colorC).compare("#00ffffff") == 0);
  };

  "simplify"_test = [] {
    expect(color_util::simplify_hex("#ff223344") == "#234");
    expect(color_util::simplify_hex("#ee223344") == "#ee223344");
    expect(color_util::simplify_hex("#ff234567") == "#234567");
  };
}
