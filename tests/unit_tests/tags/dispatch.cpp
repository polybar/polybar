#include "tags/dispatch.hpp"

#include "common/test.hpp"
#include "components/logger.hpp"
#include "gmock/gmock.h"

using namespace polybar;
using namespace std;
using namespace tags;

using ::testing::_;
using ::testing::AllOf;
using ::testing::InSequence;
using ::testing::Property;
using ::testing::Return;
using ::testing::Truly;

class MockRenderer : public renderer_interface {
 public:
  MockRenderer(action_context& action_ctxt) : renderer_interface(action_ctxt){};

  MOCK_METHOD(void, render_offset, (const context& ctxt, int pixels), (override));
  MOCK_METHOD(void, render_text, (const context& ctxt, const string&& str), (override));
  MOCK_METHOD(void, change_alignment, (const context& ctxt), (override));
  MOCK_METHOD(double, get_x, (const context& ctxt), (const, override));
  MOCK_METHOD(double, get_alignment_start, (const alignment align), (const, override));
};

static auto match_fg = [](rgba c) { return Property("get_fg", &context::get_fg, c); };
static auto match_bg = [](rgba c) { return Property("get_bg", &context::get_bg, c); };
static auto match_ol = [](rgba c) { return Property("get_ol", &context::get_ol, c); };
static auto match_ul = [](rgba c) { return Property("get_ul", &context::get_ul, c); };
static auto match_font = [](int font) { return Property("get_font", &context::get_font, font); };
static auto match_overline = [](bool has) { return Property("has_overline", &context::has_overline, has); };
static auto match_underline = [](bool has) { return Property("has_underline", &context::has_underline, has); };

static auto match_align = [](alignment align) { return Property("get_alignment", &context::get_alignment, align); };
static auto match_left_align = match_align(alignment::LEFT);
static auto match_center_align = match_align(alignment::CENTER);
static auto match_right_align = match_align(alignment::RIGHT);

class TestableDispatch : public dispatch {};

class DispatchTest : public ::testing::Test {
 protected:
  unique_ptr<action_context> m_action_ctxt = make_unique<action_context>();

  unique_ptr<dispatch> m_dispatch = make_unique<dispatch>(logger(loglevel::NONE), *m_action_ctxt);

  MockRenderer r{*m_action_ctxt};
};

TEST_F(DispatchTest, ignoreFormatting) {
  {
    InSequence seq;
    EXPECT_CALL(r, render_offset(_, 10)).Times(1);
    EXPECT_CALL(r, render_text(_, string{"abc"})).Times(1);
    EXPECT_CALL(r, render_text(_, string{"foo"})).Times(1);
  }

  bar_settings settings;
  m_dispatch->parse(settings, r, "%{O10}%{F#ff0000}abc%{F-}foo");
}

TEST_F(DispatchTest, formatting) {
  rgba bar_fg{"#000000"};

  {
    InSequence seq;
    EXPECT_CALL(r, render_offset(match_underline(true), 10)).Times(1);
    EXPECT_CALL(r, render_text(match_fg(rgba{"#ff0000"}), string{"abc"})).Times(1);
    EXPECT_CALL(r, render_text(match_fg(bar_fg), string{"foo"})).Times(1);
    EXPECT_CALL(r, change_alignment(match_left_align)).Times(1);
    EXPECT_CALL(r, render_text(AllOf(match_left_align, match_bg(rgba{"#00ff00"}), match_overline(true)), string{"bar"}))
        .Times(1);
    EXPECT_CALL(r, change_alignment(match_center_align)).Times(1);
    EXPECT_CALL(r, render_text(AllOf(match_center_align, match_ul(rgba{"#0000ff"})), string{"baz"})).Times(1);
    EXPECT_CALL(r, change_alignment(match_right_align)).Times(1);
    EXPECT_CALL(r, render_text(AllOf(match_right_align, match_ol(rgba{"#f0f0f0"}), match_font(123)), string{"def"}))
        .Times(1);
  }

  bar_settings settings;
  settings.foreground = bar_fg;

  m_dispatch->parse(settings, r,
      "%{+u}%{O10}%{F#ff0000}abc%{F-}foo%{l}%{+o}%{B#00ff00}bar%{c}%{u#0000ff}baz%{r}%{o#f0f0f0}%{T123}def");
}

TEST_F(DispatchTest, unclosedActions) {
  { InSequence seq; }

  bar_settings settings;
  EXPECT_THROW(m_dispatch->parse(settings, r, "%{A1:cmd:}foo"), runtime_error);
}

TEST_F(DispatchTest, actions) {
  {
    InSequence seq;
    EXPECT_CALL(r, get_x(_)).WillOnce(Return(3));
    EXPECT_CALL(r, get_x(_)).WillOnce(Return(6));
  }

  bar_settings settings;
  m_dispatch->parse(settings, r, "%{l}foo%{A1:cmd:}bar%{A}");

  const auto& actions = m_action_ctxt->get_blocks();

  ASSERT_EQ(1, actions.size());

  const auto& blk = actions[0];

  EXPECT_EQ(3, blk.start_x);
  EXPECT_EQ(6, blk.end_x);
  EXPECT_EQ(alignment::LEFT, blk.align);
  EXPECT_EQ(mousebtn::LEFT, blk.button);
  EXPECT_EQ("cmd", blk.cmd);
}
