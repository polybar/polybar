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

    expect(rgb{0xFF112233}.b == 0x33 / 255.0);
    expect(rgb{0x88449933}.g == 0x51 / 255.0);
    expect(rgb{0xee111111} == 0xff0f0f0f);
    expect(rgb{0x99112233} == 0xff0a141e);
  };

  "rgba"_test = []{
    unsigned int color{0xCC123456};
    expect(color_util::alpha_channel<unsigned short int>(color) == 0xCCCC);
    expect(color_util::red_channel<unsigned short int>(color) == 0x1212);
    expect(color_util::red_channel<unsigned char>(color) == 0x12);
    expect(color_util::green_channel<unsigned short int>(color) == 0x3434);
    expect(color_util::blue_channel<unsigned short int>(color) == 0x5656);

    expect(rgba{0xCC112233}.a == 0xCC / 255.0);
    expect(rgba{0x88449933}.g == 0x99 / 255.0);
    expect(static_cast<unsigned int>(rgba{0xFF111111}) == 0xFF111111);
    expect(static_cast<unsigned int>(rgba{0x00FFFFFF}) == 0x00FFFFFF);
  };

  "hex"_test = [] {
    unsigned int colorA{0x123456};
    expect(color_util::hex<unsigned char>(colorA) == "#123456"s);
    unsigned int colorB{0xCC123456};
    expect(color_util::hex<unsigned short int>(colorB) == "#cc123456"s);
    unsigned int colorC{0x00ffffff};
    expect(color_util::hex<unsigned short int>(colorC) == "#00ffffff"s);
  };

  "parse_hex"_test = [] {
    expect(color_util::parse_hex("#fff") == "#ffffffff");
    expect(color_util::parse_hex("#123") == "#ff112233");
    expect(color_util::parse_hex("#888888") == "#ff888888");
    expect(color_util::parse_hex("#00aa00aa") == "#00aa00aa");
  };

  "parse"_test = [] {
    expect(color_util::parse("invalid") == 0);
    expect(color_util::parse("#f") == 0);
    expect(color_util::parse("#ff") == 0);
    expect(color_util::parse("invalid", 0xFF999999) == 0xFF999999);
    expect(color_util::parse("invalid", 0x00111111) == 0x00111111);
    expect(color_util::parse("invalid", 0xFF000000) == 0xFF000000);
    expect(color_util::parse("#fff") == 0xffffffff);
    expect(color_util::parse("#890") == 0xFF889900);
    expect(color_util::parse("#55888777") == 0x55888777);
    expect(color_util::parse("#88aaaaaa") == 0x88aaaaaa);
    expect(color_util::parse("#00aaaaaa") == 0x00aaaaaa);
    expect(color_util::parse("#00FFFFFF") == 0x00FFFFFF);
  };

  "simplify"_test = [] {
    expect(color_util::simplify_hex("#FF111111") == "#111");
    expect(color_util::simplify_hex("#ff223344") == "#234");
    expect(color_util::simplify_hex("#ee223344") == "#ee223344");
    expect(color_util::simplify_hex("#ff234567") == "#234567");
    expect(color_util::simplify_hex("#00223344") == "#00223344");
  };
}
