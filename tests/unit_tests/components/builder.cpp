#include "components/builder.hpp"

#include "common/test.hpp"

using namespace polybar;

static const rgba FG = rgba("#00FF00");
static const rgba BG = rgba("#FF0000");

class BuilderTest : public ::testing::Test {
 protected:
  virtual void SetUp() override {
    opts.foreground = FG;
    opts.background = BG;
    opts.spacing = ZERO_SPACE;
  }

  bar_settings opts{};
  builder b{opts};
};

TEST_F(BuilderTest, empty) {
  EXPECT_EQ("", b.flush());
}

TEST_F(BuilderTest, text) {
  b.node("foo");
  EXPECT_EQ("foo", b.flush());
}

TEST_F(BuilderTest, textFont) {
  b.node("foo", 12);
  EXPECT_EQ("%{T12}foo%{T-}", b.flush());
}

using offset_test = std::pair<extent_val, string>;
class OffsetTest : public BuilderTest, public ::testing::WithParamInterface<offset_test> {};

vector<offset_test> offset_test_list = {
    {ZERO_PX_EXTENT, ""},
    {{extent_type::POINT, 0}, ""},
    {{extent_type::PIXEL, 10}, "%{O10px}"},
    {{extent_type::PIXEL, -10}, "%{O-10px}"},
    {{extent_type::POINT, 23}, "%{O23pt}"},
    {{extent_type::POINT, -1}, "%{O-1pt}"},
};

INSTANTIATE_TEST_SUITE_P(Inst, OffsetTest, ::testing::ValuesIn(offset_test_list));

TEST_P(OffsetTest, correctness) {
  b.offset(GetParam().first);
  EXPECT_EQ(GetParam().second, b.flush());
}

using spacing_test = std::pair<spacing_val, string>;
class SpacingTest : public BuilderTest, public ::testing::WithParamInterface<spacing_test> {};

vector<spacing_test> spacing_test_list = {
    {ZERO_SPACE, ""},
    {{spacing_type::SPACE, 2}, "  "},
    {{spacing_type::PIXEL, 3}, "%{O3px}"},
    {{spacing_type::POINT, 4}, "%{O4pt}"},
};

INSTANTIATE_TEST_SUITE_P(Inst, SpacingTest, ::testing::ValuesIn(spacing_test_list));

TEST_P(SpacingTest, correctness) {
  b.spacing(GetParam().first);
  EXPECT_EQ(GetParam().second, b.flush());
}

TEST_P(SpacingTest, get_spacing_format_string) {
  b.spacing(GetParam().first);
  EXPECT_EQ(GetParam().second, b.flush());
}

TEST_F(BuilderTest, tags) {
  b.font(10);
  b.font_close();
  EXPECT_EQ("%{T10}%{T-}", b.flush());

  b.background(rgba("#f0f0f0"));
  b.background_close();
  EXPECT_EQ("%{B#f0f0f0}%{B-}", b.flush());

  b.foreground(rgba("#0f0f0f"));
  b.foreground_close();
  EXPECT_EQ("%{F#0f0f0f}%{F-}", b.flush());

  b.overline(rgba("#0e0e0e"));
  b.overline_close();
  // Last tag is from auto-closing during flush
  EXPECT_EQ("%{o#0e0e0e}%{+o}%{-o}%{o-}", b.flush());

  b.underline(rgba("#0d0d0d"));
  b.underline_close();
  // Last tag is from auto-closing during flush
  EXPECT_EQ("%{u#0d0d0d}%{+u}%{-u}%{u-}", b.flush());

  b.control(tags::controltag::R);
  EXPECT_EQ("%{PR}", b.flush());

  b.action(mousebtn::LEFT, "cmd");
  b.action_close();
  EXPECT_EQ("%{A1:cmd:}%{A}", b.flush());
}

TEST_F(BuilderTest, tagsAutoClose) {
  b.font(20);
  EXPECT_EQ("%{T20}%{T-}", b.flush());

  b.background(rgba("#f0f0f0"));
  EXPECT_EQ("%{B#f0f0f0}%{B-}", b.flush());

  b.foreground(rgba("#0f0f0f"));
  EXPECT_EQ("%{F#0f0f0f}%{F-}", b.flush());

  b.overline(rgba("#0e0e0e"));
  EXPECT_EQ("%{o#0e0e0e}%{+o}%{o-}%{-o}", b.flush());

  b.underline(rgba("#0d0d0d"));
  EXPECT_EQ("%{u#0d0d0d}%{+u}%{u-}%{-u}", b.flush());

  b.action(mousebtn::LEFT, "cmd");
  EXPECT_EQ("%{A1:cmd:}%{A}", b.flush());
}

TEST_F(BuilderTest, invalidTags) {
  EXPECT_THROW(b.control(tags::controltag::NONE), std::runtime_error);
}

TEST_F(BuilderTest, colorBlending) {
  b.background(rgba(0x12000000, rgba::type::ALPHA_ONLY));
  b.background_close();

  EXPECT_EQ("%{B#12ff0000}%{B-}", b.flush());

  b.foreground(rgba(0x34000000, rgba::type::ALPHA_ONLY));
  b.foreground_close();

  EXPECT_EQ("%{F#3400ff00}%{F-}", b.flush());
}
