#include "tags/parser.hpp"

#include "common/test.hpp"

using namespace polybar;
using namespace tags;

/**
 * Helper class to test parsed data.
 *
 * The expect_* functions will check that the current element corresponds to
 * what is expected and then move to the next element.
 *
 * The assert_* functions are used internally to check certain properties of the
 * current element.
 */
class TestableTagParser : public parser {
 public:
  TestableTagParser() : TestableTagParser(""){};

  TestableTagParser(const string&& input) {
    setup_parser_test(std::move(input));
  }

  void setup_parser_test(const string& input) {
    this->set(std::move(input));
  }

  void expect_done() {
    EXPECT_FALSE(has_next_element());
  }

  void expect_text(const string&& exp) {
    set_current();
    assert_is_tag(false);
    EXPECT_EQ(exp, current.data);
  }

  void expect_color_reset(syntaxtag t) {
    set_current();
    assert_format(t);
    EXPECT_EQ(color_type::RESET, current.tag_data.color.type);
  }

  void expect_color(syntaxtag t, const string& color) {
    set_current();
    assert_format(t);
    rgba c{color};
    EXPECT_EQ(color_type::COLOR, current.tag_data.color.type);
    EXPECT_EQ(c, current.tag_data.color.val);
  }

  void expect_action_closing(mousebtn exp = mousebtn::NONE) {
    set_current();
    assert_format(syntaxtag::A);
    EXPECT_TRUE(current.tag_data.action.closing);
    EXPECT_EQ(exp, current.tag_data.action.btn);
  }

  void expect_action(const string& exp, mousebtn btn) {
    set_current();
    assert_format(syntaxtag::A);
    EXPECT_FALSE(current.tag_data.action.closing);
    EXPECT_EQ(btn, current.tag_data.action.btn);
    EXPECT_EQ(exp, current.data);
  }

  void expect_font_reset() {
    expect_font(0);
  }

  void expect_font(unsigned exp) {
    set_current();
    assert_format(syntaxtag::T);
    EXPECT_EQ(exp, current.tag_data.font);
  }

  void expect_offset_pixel(int exp) {
    set_current();
    assert_format(syntaxtag::O);
    EXPECT_EQ(extent_type::PIXEL, current.tag_data.offset.type);
    EXPECT_EQ(exp, current.tag_data.offset.value);
  }

  void expect_offset_points(float exp) {
    set_current();
    assert_format(syntaxtag::O);
    EXPECT_EQ(extent_type::POINT, current.tag_data.offset.type);
    EXPECT_EQ(exp, current.tag_data.offset.value);
  }

  void expect_ctrl(controltag exp) {
    set_current();
    assert_format(syntaxtag::P);
    EXPECT_EQ(exp, current.tag_data.ctrl);
  }

  void expect_alignment(syntaxtag exp) {
    set_current();
    assert_format(exp);
  }

  void expect_activation(attr_activation act, attribute attr) {
    set_current();
    assert_type(tag_type::ATTR);
    EXPECT_EQ(act, current.tag_data.subtype.activation);
    EXPECT_EQ(attr, current.tag_data.attr);
  }

  void expect_reverse() {
    set_current();
    assert_format(syntaxtag::R);
  }

 private:
  void assert_format(syntaxtag exp) {
    assert_type(tag_type::FORMAT);
    ASSERT_EQ(exp, current.tag_data.subtype.format);
  }

  void assert_type(tag_type exp) {
    assert_is_tag(true);
    ASSERT_EQ(exp, current.tag_data.type);
  }

  void assert_is_tag(bool exp) {
    ASSERT_EQ(exp, current.is_tag);
  }

  void assert_has() {
    if (!has_next_element()) {
      throw std::runtime_error("no next element");
    }
  }

  void set_current() {
    assert_has();
    current = next_element();
  }

  element current;
};

class TagParserTest : public ::testing::Test {
 protected:
  TestableTagParser p;
};

TEST_F(TagParserTest, empty) {
  p.setup_parser_test("");
  p.expect_done();
}

TEST_F(TagParserTest, text) {
  p.setup_parser_test("text");
  p.expect_text("text");
  p.expect_done();
}

// Single Tag {{{

// Parse Single Color {{{
/**
 * <input, tag, colorstring>
 *
 * If the color string is empty, this is supposed to be a color reset
 */
using single_color = std::tuple<string, syntaxtag, string>;

class ParseSingleColorTest : public TagParserTest, public ::testing::WithParamInterface<single_color> {};

vector<single_color> parse_single_color_list = {
    {"%{B-}", syntaxtag::B, ""},
    {"%{F-}", syntaxtag::F, ""},
    {"%{o-}", syntaxtag::o, ""},
    {"%{u-}", syntaxtag::u, ""},
    {"%{B}", syntaxtag::B, ""},
    {"%{F}", syntaxtag::F, ""},
    {"%{o}", syntaxtag::o, ""},
    {"%{u}", syntaxtag::u, ""},
    {"%{B#f0f0f0}", syntaxtag::B, "#f0f0f0"},
    {"%{F#abc}", syntaxtag::F, "#abc"},
    {"%{o#abcd}", syntaxtag::o, "#abcd"},
    {"%{u#FDE}", syntaxtag::u, "#FDE"},
    {"%{    u#FDE}", syntaxtag::u, "#FDE"},
};

INSTANTIATE_TEST_SUITE_P(Inst, ParseSingleColorTest, ::testing::ValuesIn(parse_single_color_list));

TEST_P(ParseSingleColorTest, correctness) {
  string input;
  syntaxtag t;
  string color;
  std::tie(input, t, color) = GetParam();

  p.setup_parser_test(std::move(input));

  if (color.empty()) {
    p.expect_color_reset(t);
  } else {
    p.expect_color(t, color);
  }

  p.expect_done();
}

// }}}

// Parse Single Action {{{

/**
 * If the third element is an empty string, this is a closing tag.
 */
using single_action = std::tuple<string, mousebtn, string>;

class ParseSingleActionTest : public TagParserTest, public ::testing::WithParamInterface<single_action> {};

vector<single_action> parse_single_action_list = {
    {"%{A:cmd:}", mousebtn::LEFT, "cmd"},
    {"%{A1:cmd:}", mousebtn::LEFT, "cmd"},
    {"%{A2:cmd:}", mousebtn::MIDDLE, "cmd"},
    {"%{A3:cmd:}", mousebtn::RIGHT, "cmd"},
    {"%{A4:cmd:}", mousebtn::SCROLL_UP, "cmd"},
    {"%{A5:cmd:}", mousebtn::SCROLL_DOWN, "cmd"},
    {"%{A6:cmd:}", mousebtn::DOUBLE_LEFT, "cmd"},
    {"%{A7:cmd:}", mousebtn::DOUBLE_MIDDLE, "cmd"},
    {"%{A8:cmd:}", mousebtn::DOUBLE_RIGHT, "cmd"},
    {"%{A}", mousebtn::NONE, ""},
    {"%{A1}", mousebtn::LEFT, ""},
    {"%{A2}", mousebtn::MIDDLE, ""},
    {"%{A3}", mousebtn::RIGHT, ""},
    {"%{A4}", mousebtn::SCROLL_UP, ""},
    {"%{A5}", mousebtn::SCROLL_DOWN, ""},
    {"%{A6}", mousebtn::DOUBLE_LEFT, ""},
    {"%{A7}", mousebtn::DOUBLE_MIDDLE, ""},
    {"%{A8}", mousebtn::DOUBLE_RIGHT, ""},
    {"%{A1:a\\:b:}", mousebtn::LEFT, "a:b"},
    {"%{A1:\\:\\:\\::}", mousebtn::LEFT, ":::"},
    {"%{A1:#apps.open.0:}", mousebtn::LEFT, "#apps.open.0"},
    // https://github.com/polybar/polybar/issues/2040
    {"%{A1:cmd | awk '{ print $NF }'):}", mousebtn::LEFT, "cmd | awk '{ print $NF }')"},
};

INSTANTIATE_TEST_SUITE_P(Inst, ParseSingleActionTest, ::testing::ValuesIn(parse_single_action_list));

TEST_P(ParseSingleActionTest, correctness) {
  string input;
  mousebtn btn;
  string cmd;
  std::tie(input, btn, cmd) = GetParam();

  p.setup_parser_test(std::move(input));

  if (cmd.empty()) {
    p.expect_action_closing(btn);
  } else {
    p.expect_action(cmd, btn);
  }

  p.expect_done();
}

// }}}

// Parse Single Activation {{{
using single_activation = std::tuple<string, attribute, attr_activation>;

class ParseSingleActivationTest : public TagParserTest, public ::testing::WithParamInterface<single_activation> {};

vector<single_activation> parse_single_activation_list = {
    {"%{+u}", attribute::UNDERLINE, attr_activation::ON},
    {"%{-u}", attribute::UNDERLINE, attr_activation::OFF},
    {"%{!u}", attribute::UNDERLINE, attr_activation::TOGGLE},
    {"%{+o}", attribute::OVERLINE, attr_activation::ON},
    {"%{-o}", attribute::OVERLINE, attr_activation::OFF},
    {"%{!o}", attribute::OVERLINE, attr_activation::TOGGLE},
};

INSTANTIATE_TEST_SUITE_P(Inst, ParseSingleActivationTest, ::testing::ValuesIn(parse_single_activation_list));

TEST_P(ParseSingleActivationTest, correctness) {
  string input;
  attribute attr;
  attr_activation act;
  std::tie(input, attr, act) = GetParam();
  p.setup_parser_test(std::move(input));
  p.expect_activation(act, attr);
  p.expect_done();
}

// }}}

TEST_F(TagParserTest, reverse) {
  p.setup_parser_test("%{R}");
  p.expect_reverse();
  p.expect_done();
}

TEST_F(TagParserTest, font) {
  p.setup_parser_test("%{T}");
  p.expect_font_reset();
  p.expect_done();

  p.setup_parser_test("%{T-}");
  p.expect_font_reset();
  p.expect_done();

  p.setup_parser_test("%{T-123}");
  p.expect_font_reset();
  p.expect_done();

  p.setup_parser_test("%{T123}");
  p.expect_font(123);
  p.expect_done();
}

TEST_F(TagParserTest, offset) {
  p.setup_parser_test("%{O}");
  p.expect_offset_pixel(0);
  p.expect_done();

  p.setup_parser_test("%{O0}");
  p.expect_offset_pixel(0);
  p.expect_done();

  p.setup_parser_test("%{O-112}");
  p.expect_offset_pixel(-112);
  p.expect_done();

  p.setup_parser_test("%{O123}");
  p.expect_offset_pixel(123);
  p.expect_done();

  p.setup_parser_test("%{O0pt}");
  p.expect_offset_points(0);
  p.expect_done();

  p.setup_parser_test("%{O-112pt}");
  p.expect_offset_points(-112);
  p.expect_done();

  p.setup_parser_test("%{O123pt}");
  p.expect_offset_points(123);
  p.expect_done();

  p.setup_parser_test("%{O1.5pt}");
  p.expect_offset_points(1.5);
  p.expect_done();

  p.setup_parser_test("%{O1.1px}");
  p.expect_offset_pixel(1);
  p.expect_done();

  p.setup_parser_test("%{O1.1}");
  p.expect_offset_pixel(1);
  p.expect_done();
}

TEST_F(TagParserTest, alignment) {
  p.setup_parser_test("%{l}");
  p.expect_alignment(syntaxtag::l);
  p.expect_done();

  p.setup_parser_test("%{c}");
  p.expect_alignment(syntaxtag::c);
  p.expect_done();

  p.setup_parser_test("%{r}");
  p.expect_alignment(syntaxtag::r);
  p.expect_done();
}

TEST_F(TagParserTest, ctrl) {
  p.setup_parser_test("%{PR}");
  p.expect_ctrl(controltag::R);
  p.expect_done();
}

/**
 * Tests the the legacy %{U...} tag first produces %{u...} and then %{o...}
 */
TEST_F(TagParserTest, UnderOverLine) {
  p.setup_parser_test("%{U-}");
  p.expect_color_reset(syntaxtag::u);
  p.expect_color_reset(syntaxtag::o);
  p.expect_done();

  p.setup_parser_test("%{U#12ab}");
  p.expect_color(syntaxtag::u, "#12ab");
  p.expect_color(syntaxtag::o, "#12ab");
  p.expect_done();
}

// }}}

TEST_F(TagParserTest, compoundTags) {
  p.setup_parser_test("%{F-  B#ff0000    A:cmd:}");
  p.expect_color_reset(syntaxtag::F);
  p.expect_color(syntaxtag::B, "#ff0000");
  p.expect_action("cmd", mousebtn::LEFT);
  p.expect_done();
}

TEST_F(TagParserTest, combinations) {
  p.setup_parser_test("%{r}%{u#4bffdc +u u#4bffdc} 20% abc%{-u u- PR}");
  p.expect_alignment(syntaxtag::r);
  p.expect_color(syntaxtag::u, "#4bffdc");
  p.expect_activation(attr_activation::ON, attribute::UNDERLINE);
  p.expect_color(syntaxtag::u, "#4bffdc");
  p.expect_text(" 20% abc");
  p.expect_activation(attr_activation::OFF, attribute::UNDERLINE);
  p.expect_color_reset(syntaxtag::u);
  p.expect_ctrl(controltag::R);
  p.expect_done();
}

/**
 * The type of exception we expect.
 *
 * Since we can't directly pass typenames, we go through this enum.
 */
enum class exc { ERR, TOKEN, TAG, TAG_END, COLOR, ATTR, FONT, CTRL, OFFSET, BTN };

using exception_test = pair<string, enum exc>;
class ParseErrorTest : public TagParserTest, public ::testing::WithParamInterface<exception_test> {};

vector<exception_test> parse_error_test = {
    {"%{F-", exc::TAG_END},
    {"%{Q", exc::TAG},
    {"%{", exc::TOKEN},
    {"%{F#xyz}", exc::COLOR},
    {"%{Ffoo}", exc::COLOR},
    {"%{F-abc}", exc::COLOR},
    {"%{+z}", exc::ATTR},
    {"%{T-abc}", exc::FONT},
    {"%{T12a}", exc::FONT},
    {"%{Tabc}", exc::FONT},
    {"%{?u", exc::TAG},
    {"%{PRabc}", exc::CTRL},
    {"%{P}", exc::CTRL},
    {"%{PA}", exc::CTRL},
    {"%{Oabc}", exc::OFFSET},
    {"%{O123foo}", exc::OFFSET},
    {"%{O0ptx}", exc::OFFSET},
    {"%{O0a}", exc::OFFSET},
    {"%{A2:cmd:cmd:}", exc::TAG_END},
    {"%{A9}", exc::BTN},
    {"%{rQ}", exc::TAG_END},
};

INSTANTIATE_TEST_SUITE_P(Inst, ParseErrorTest, ::testing::ValuesIn(parse_error_test));

TEST_P(ParseErrorTest, correctness) {
  string input;
  exc exception;
  std::tie(input, exception) = GetParam();

  p.setup_parser_test(input);
  ASSERT_TRUE(p.has_next_element());

  switch (exception) {
    case exc::ERR:
      ASSERT_THROW(p.next_element(), tags::error);
      break;
    case exc::TOKEN:
      ASSERT_THROW(p.next_element(), tags::token_error);
      break;
    case exc::TAG:
      ASSERT_THROW(p.next_element(), tags::unrecognized_tag);
      break;
    case exc::TAG_END:
      ASSERT_THROW(p.next_element(), tags::tag_end_error);
      break;
    case exc::COLOR:
      ASSERT_THROW(p.next_element(), tags::color_error);
      break;
    case exc::ATTR:
      ASSERT_THROW(p.next_element(), tags::unrecognized_attr);
      break;
    case exc::FONT:
      ASSERT_THROW(p.next_element(), tags::font_error);
      break;
    case exc::CTRL:
      ASSERT_THROW(p.next_element(), tags::control_error);
      break;
    case exc::OFFSET:
      ASSERT_THROW(p.next_element(), tags::offset_error);
      break;
    case exc::BTN:
      ASSERT_THROW(p.next_element(), tags::btn_error);
      break;
    default:
      FAIL();
  }
}
