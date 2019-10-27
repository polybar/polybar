#include "utils/color.hpp"

#include "common/test.hpp"

using namespace polybar;

TEST(Rgba, constructor) {
  rgba v{"invalid"};
  EXPECT_FALSE(v.has_color());

  v = rgba{"#f"};
  EXPECT_FALSE(v.has_color());

  v = rgba{"#12"};
  EXPECT_EQ(rgba::ALPHA_ONLY, v.m_type);

  v = rgba{"#ff"};
  EXPECT_EQ(0xff000000, v.m_value);

  v = rgba{"#fff"};
  EXPECT_EQ(0xffffffff, v.m_value);

  v = rgba{"#890"};
  EXPECT_EQ(0xFF889900, v.m_value);

  v = rgba{"#a890"};
  EXPECT_EQ(0xaa889900, v.m_value);

  v = rgba{"#55888777"};
  EXPECT_EQ(0x55888777, v.m_value);

  v = rgba{"#88aaaaaa"};
  EXPECT_EQ(0x88aaaaaa, v.m_value);

  v = rgba{"#00aaaaaa"};
  EXPECT_EQ(0x00aaaaaa, v.m_value);

  v = rgba{"#00FFFFFF"};
  EXPECT_EQ(0x00FFFFFF, v.m_value);
}

TEST(Rgba, parse) {
  EXPECT_EQ(0xffffffff, rgba{"#fff"}.m_value);
  EXPECT_EQ(0xffffffff, rgba{"fff"}.m_value);
  EXPECT_EQ(0xff112233, rgba{"#123"}.m_value);
  EXPECT_EQ(0xff112233, rgba{"123"}.m_value);
  EXPECT_EQ(0xff888888, rgba{"#888888"}.m_value);
  EXPECT_EQ(0xff888888, rgba{"888888"}.m_value);
  EXPECT_EQ(0x00aa00aa, rgba{"#00aa00aa"}.m_value);
  EXPECT_EQ(0x00aa00aa, rgba{"00aa00aa"}.m_value);
  EXPECT_EQ(0x11223344, rgba{"#1234"}.m_value);
  EXPECT_EQ(0x11223344, rgba{"1234"}.m_value);
  EXPECT_EQ(0xaa000000, rgba{"#aa"}.m_value);
  EXPECT_EQ(0xaa000000, rgba{"aa"}.m_value);
}

TEST(Rgba, string) {
  rgba v{"#1234"};

  EXPECT_EQ("#11223344", static_cast<string>(v));

  v = rgba{"#12"};

  EXPECT_EQ("#12000000", static_cast<string>(v));
}

TEST(Rgba, eq) {
  rgba v(0x12, rgba::NONE);

  EXPECT_TRUE(v == rgba(0, rgba::NONE));
  EXPECT_TRUE(v == rgba(0x11, rgba::NONE));
  EXPECT_FALSE(v == rgba{0x1234});

  v = rgba{0xCC123456};

  EXPECT_TRUE(v == rgba{0xCC123456});
  EXPECT_FALSE(v == rgba(0xCC123456, rgba::NONE));

  v = rgba{"#aa"};

  EXPECT_TRUE(v == rgba(0xaa000000, rgba::ALPHA_ONLY));
  EXPECT_FALSE(v == rgba(0xaa000000, rgba::ARGB));
  EXPECT_FALSE(v == rgba(0xab000000, rgba::ALPHA_ONLY));
}

TEST(Rgba, hasColor) {
  rgba v{"#"};

  EXPECT_FALSE(v.has_color());

  v = rgba{"#ff"};

  EXPECT_TRUE(v.has_color());

  v = rgba{"#cc123456"};

  EXPECT_TRUE(v.has_color());

  v = rgba(0x1243, rgba::NONE);

  EXPECT_FALSE(v.has_color());
}

TEST(ColorUtil, rgba) {
  uint32_t color{0xCC123456};
  EXPECT_EQ(0xCC, color_util::alpha_channel(color));
  EXPECT_EQ(0x12, color_util::red_channel(color));
  EXPECT_EQ(0x34, color_util::green_channel(color));
  EXPECT_EQ(0x56, color_util::blue_channel(color));

  EXPECT_EQ(0xCC / 255.0, rgba{0xCC112233}.a());
  EXPECT_EQ(0x99 / 255.0, rgba{0x88449933}.g());
  EXPECT_EQ(0xFF111111, static_cast<uint32_t>(rgba{"#FF111111"}));
  EXPECT_EQ(0x00FFFFFF, static_cast<uint32_t>(rgba{"#00FFFFFF"}));
}

TEST(ColorUtil, hex) {
  uint32_t colorA{0x123456};
  EXPECT_EQ("#00123456"s, color_util::hex(colorA));
  uint32_t colorB{0xCC123456};
  EXPECT_EQ("#cc123456"s, color_util::hex(colorB));
  uint32_t colorC{0x00ffffff};
  EXPECT_EQ("#00ffffff"s, color_util::hex(colorC));
}

TEST(ColorUtil, simplify) {
  EXPECT_EQ("#111", color_util::simplify_hex("#FF111111"));
  EXPECT_EQ("#234", color_util::simplify_hex("#ff223344"));
  EXPECT_EQ("#ee223344", color_util::simplify_hex("#ee223344"));
  EXPECT_EQ("#234567", color_util::simplify_hex("#ff234567"));
  EXPECT_EQ("#00223344", color_util::simplify_hex("#00223344"));
}
