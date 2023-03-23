#include "utils/color.hpp"

#include "common/test.hpp"

using namespace polybar;

TEST(Rgba, constructor) {
  EXPECT_FALSE(rgba("invalid").has_color());

  EXPECT_FALSE(rgba("#f").has_color());

  EXPECT_FALSE(rgba("#-abc").has_color());
  EXPECT_FALSE(rgba("#xyz").has_color());

  EXPECT_EQ(rgba::type::ALPHA_ONLY, rgba{"#12"}.get_type());

  EXPECT_EQ(0xff000000, rgba{"#ff"}.value());

  EXPECT_EQ(0xffffffff, rgba{"#fff"}.value());

  EXPECT_EQ(0xFF889900, rgba{"#890"}.value());

  EXPECT_EQ(0xaa889900, rgba{"#a890"}.value());

  EXPECT_EQ(0x55888777, rgba{"#55888777"}.value());

  EXPECT_EQ(0x88aaaaaa, rgba{"#88aaaaaa"}.value());

  EXPECT_EQ(0x00aaaaaa, rgba{"#00aaaaaa"}.value());

  EXPECT_EQ(0x00FFFFFF, rgba{"#00FFFFFF"}.value());
}

TEST(Rgba, parse) {
  EXPECT_EQ(0xffffffff, rgba{"#fff"}.value());
  EXPECT_EQ(0xffffffff, rgba{"fff"}.value());
  EXPECT_EQ(0xff112233, rgba{"#123"}.value());
  EXPECT_EQ(0xff112233, rgba{"123"}.value());
  EXPECT_EQ(0xff888888, rgba{"#888888"}.value());
  EXPECT_EQ(0xff888888, rgba{"888888"}.value());
  EXPECT_EQ(0x00aa00aa, rgba{"#00aa00aa"}.value());
  EXPECT_EQ(0x00aa00aa, rgba{"00aa00aa"}.value());
  EXPECT_EQ(0x11223344, rgba{"#1234"}.value());
  EXPECT_EQ(0x11223344, rgba{"1234"}.value());
  EXPECT_EQ(0xaa000000, rgba{"#aa"}.value());
  EXPECT_EQ(0xaa000000, rgba{"aa"}.value());
}

TEST(Rgba, string) {
  EXPECT_EQ("#11223344", static_cast<string>(rgba{"#1234"}));
  EXPECT_EQ("#12000000", static_cast<string>(rgba{"#12"}));
}

TEST(Rgba, eq) {
  rgba v(0x12, rgba::type::NONE);

  EXPECT_TRUE(v == rgba(0, rgba::type::NONE));
  EXPECT_TRUE(v == rgba(0x11, rgba::type::NONE));
  EXPECT_FALSE(v == rgba{0x123456});

  v = rgba{0xCC123456};

  EXPECT_TRUE(v == rgba{0xCC123456});
  EXPECT_FALSE(v == rgba(0xCC123456, rgba::type::NONE));

  v = rgba{"#aa"};

  EXPECT_TRUE(v == rgba(0xaa000000, rgba::type::ALPHA_ONLY));
  EXPECT_FALSE(v == rgba(0xaa000000, rgba::type::ARGB));
  EXPECT_FALSE(v == rgba(0xab000000, rgba::type::ALPHA_ONLY));
}

TEST(Rgba, hasColor) {
  rgba v{"#"};

  EXPECT_FALSE(v.has_color());

  v = rgba{"#ff"};

  EXPECT_TRUE(v.has_color());

  v = rgba{"#cc123456"};

  EXPECT_TRUE(v.has_color());

  v = rgba(0x1243, rgba::type::NONE);

  EXPECT_FALSE(v.has_color());
}

TEST(Rgba, isTransparent) {
  rgba v{"#123456"};
  EXPECT_FALSE(v.is_transparent());

  v = rgba{"#ff123456"};

  EXPECT_FALSE(v.is_transparent());

  v = rgba{"#fe123456"};

  EXPECT_TRUE(v.is_transparent());
}

TEST(Rgba, channel) {
  rgba v{0xCC123456};
  EXPECT_EQ(0xCC, v.alpha_i());
  EXPECT_EQ(0x12, v.red_i());
  EXPECT_EQ(0x34, v.green_i());
  EXPECT_EQ(0x56, v.blue_i());

  EXPECT_EQ(0xCC / 255.0, rgba{0xCC112233}.alpha_d());
  EXPECT_EQ(0x99 / 255.0, rgba{0x88449933}.green_d());
}

TEST(Rgba, applyAlphaTo) {
  rgba v{0xAA000000, rgba::type::ALPHA_ONLY};
  rgba modified = v.apply_alpha_to(rgba{0xCC123456});
  EXPECT_EQ(0xAA123456, modified.value());

  v = rgba{0xCC999999};
  modified = v.apply_alpha_to(rgba{0x00123456});
  EXPECT_EQ(0xCC123456, modified.value());
}

TEST(Rgba, tryApplyAlphaTo) {
  rgba v{0xAA000000, rgba::type::ALPHA_ONLY};
  rgba modified = v.try_apply_alpha_to(rgba{0xCC123456});
  EXPECT_EQ(0xAA123456, modified.value());

  v = rgba{0xCC999999};
  modified = v.try_apply_alpha_to(rgba{0x00123456});
  EXPECT_EQ(0xCC999999, modified.value());
}

TEST(ColorUtil, simplify) {
  EXPECT_EQ("#111", color_util::simplify_hex("#FF111111"));
  EXPECT_EQ("#234", color_util::simplify_hex("#ff223344"));
  EXPECT_EQ("#ee223344", color_util::simplify_hex("#ee223344"));
  EXPECT_EQ("#234567", color_util::simplify_hex("#ff234567"));
  EXPECT_EQ("#00223344", color_util::simplify_hex("#00223344"));
}
