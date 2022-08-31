#include "tags/dispatch.hpp"

#include "common/test.hpp"
#include "components/logger.hpp"
#include "events/signal_emitter.hpp"
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

namespace polybar {
  inline bool operator==(const extent_val& a, const extent_val& b) {
    return a.type == b.type && a.value == b.value;
  }
} // namespace polybar

/**
 * Fake renderer that just tracks the current x-position per alignment.
 *
 * Each text character and point is treated as a single pixel.
 */
class FakeRenderer : public renderer_interface {
 public:
  FakeRenderer(action_context& action_ctxt) : renderer_interface(action_ctxt){};

  void render_offset(const tags::context& ctxt, const extent_val offset) override {
    EXPECT_NE(alignment::NONE, ctxt.get_alignment());
    block_x[ctxt.get_alignment()] += offset.value;
  };

  void render_text(const tags::context& ctxt, const string&& str) override {
    EXPECT_NE(alignment::NONE, ctxt.get_alignment());
    block_x[ctxt.get_alignment()] += str.size();
  };

  void change_alignment(const tags::context& ctxt) override {
    EXPECT_NE(alignment::NONE, ctxt.get_alignment());
  };

  double get_x(const tags::context& ctxt) const override {
    EXPECT_NE(alignment::NONE, ctxt.get_alignment());
    return block_x.at(ctxt.get_alignment());
  };

  double get_alignment_start(const alignment) const override {
    return 0;
  };

  void apply_tray_position(const tags::context&) override{};

 private:
  map<alignment, int> block_x = {
      {alignment::LEFT, 0},
      {alignment::CENTER, 0},
      {alignment::RIGHT, 0},
  };
};

class MockRenderer : public renderer_interface {
 public:
  MockRenderer(action_context& action_ctxt) : renderer_interface(action_ctxt), fake(action_ctxt){};

  MOCK_METHOD(void, render_offset, (const context& ctxt, const extent_val offset), (override));
  MOCK_METHOD(void, render_text, (const context& ctxt, const string&& str), (override));
  MOCK_METHOD(void, change_alignment, (const context& ctxt), (override));
  MOCK_METHOD(double, get_x, (const context& ctxt), (const, override));
  MOCK_METHOD(double, get_alignment_start, (const alignment align), (const, override));
  MOCK_METHOD(void, apply_tray_position, (const polybar::tags::context& context), (override));

  void DelegateToFake() {
    ON_CALL(*this, render_offset).WillByDefault([this](const context& ctxt, const extent_val offset) {
      fake.render_offset(ctxt, offset);
    });

    ON_CALL(*this, render_text).WillByDefault([this](const context& ctxt, const string&& str) {
      fake.render_text(ctxt, std::forward<const string>(str));
    });

    ON_CALL(*this, get_x).WillByDefault([this](const context& ctxt) { return fake.get_x(ctxt); });

    ON_CALL(*this, get_alignment_start).WillByDefault([this](const alignment a) {
      return fake.get_alignment_start(a);
    });

    ON_CALL(*this, apply_tray_position).WillByDefault([this](const context& context) {
      return fake.apply_tray_position(context);
    });
  }

 private:
  FakeRenderer fake;
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

class DispatchTest : public ::testing::Test {
 protected:
  unique_ptr<action_context> m_action_ctxt = make_unique<action_context>();

  unique_ptr<dispatch> m_dispatch = make_unique<dispatch>(logger(loglevel::NONE), *m_action_ctxt);

  ::testing::NiceMock<MockRenderer> r{*m_action_ctxt};
  void SetUp() override {
    r.DelegateToFake();
  }
};

TEST_F(DispatchTest, ignoreFormatting) {
  {
    InSequence seq;
    EXPECT_CALL(r, render_offset(_, extent_val{extent_type::PIXEL, 10})).Times(1);
    EXPECT_CALL(r, render_text(_, string{"abc"})).Times(1);
    EXPECT_CALL(r, render_text(_, string{"foo"})).Times(1);
  }

  bar_settings settings;
  m_dispatch->parse(settings, r, "%{l}%{O10}%{F#ff0000}abc%{F-}foo");
}

TEST_F(DispatchTest, formatting) {
  bar_settings settings;
  rgba bar_fg = settings.foreground;
  rgba bar_bg = settings.background;

  rgba c1{"#ff0000"};
  rgba c2{"#00ff00"};
  rgba c3{"#0000ff"};
  rgba c4{"#f0f0f0"};

  {
    InSequence seq;
    EXPECT_CALL(r, change_alignment(match_left_align)).Times(1);
    EXPECT_CALL(r, render_offset(_, extent_val{extent_type::PIXEL, 10})).Times(1);
    EXPECT_CALL(r, render_text(match_fg(c1), string{"abc"})).Times(1);
    EXPECT_CALL(r, render_text(match_fg(bar_fg), string{"foo"})).Times(1);
    EXPECT_CALL(r, change_alignment(match_left_align)).Times(1);
    EXPECT_CALL(r, render_text(AllOf(match_left_align, match_bg(c2), match_fg(bar_fg)), string{"bar"})).Times(1);
    EXPECT_CALL(r, render_text(AllOf(match_left_align, match_fg(c2), match_bg(bar_fg)), string{"123"})).Times(1);
    EXPECT_CALL(r, change_alignment(match_center_align)).Times(1);
    EXPECT_CALL(r, render_text(match_center_align, string{"baz"})).Times(1);
    EXPECT_CALL(r, change_alignment(match_right_align)).Times(1);
    EXPECT_CALL(r, render_text(AllOf(match_right_align, match_font(123)), string{"def"})).Times(1);
    EXPECT_CALL(r, render_text(AllOf(match_right_align, match_fg(bar_fg), match_bg(bar_bg)), string{"ghi"})).Times(1);
  }

  m_dispatch->parse(settings, r,
      "%{l}%{O10}%{F#ff0000}abc%{F-}foo%{l}%{B#00ff00}bar%{R}123%{c}baz%{r}%{T123}def%{PR}"
      "ghi");
}

TEST_F(DispatchTest, formattingAttributes) {
  bar_settings settings;
  rgba bar_ol = settings.overline.color;
  rgba bar_ul = settings.underline.color;

  rgba c1{"#0000ff"};
  rgba c2{"#f0f0f0"};

  {
    InSequence seq;
    EXPECT_CALL(
        r, render_text(AllOf(match_ul(c1), match_ol(c2), match_overline(true), match_underline(true)), string{"123"}))
        .Times(1);
    EXPECT_CALL(
        r, render_text(AllOf(match_ul(c1), match_ol(c2), match_overline(false), match_underline(false)), string{"456"}))
        .Times(1);
    EXPECT_CALL(
        r, render_text(AllOf(match_ul(c1), match_ol(c2), match_overline(true), match_underline(true)), string{"789"}))
        .Times(1);
    EXPECT_CALL(r, render_text(AllOf(match_ul(bar_ul), match_ol(bar_ol), match_overline(false), match_underline(false)),
                       string{"0"}))
        .Times(1);
  }

  m_dispatch->parse(settings, r, "%{l}%{u#0000ff o#f0f0f0 +o +u}123%{-u -o}456%{!u !o}789%{PR}0");
}

TEST_F(DispatchTest, unclosedActions) {
  { InSequence seq; }

  bar_settings settings;
  EXPECT_THROW(m_dispatch->parse(settings, r, "%{l}%{A1:cmd:}foo"), runtime_error);
}

TEST_F(DispatchTest, actions) {
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

TEST_F(DispatchTest, actionOffsetStart) {
  bar_settings settings;
  m_dispatch->parse(settings, r, "%{l}a%{A1:cmd:}%{O-1}bar%{A}");

  const auto& actions = m_action_ctxt->get_blocks();

  ASSERT_EQ(1, actions.size());

  const auto& blk = actions[0];

  EXPECT_EQ(0, blk.start_x);
  EXPECT_EQ(3, blk.end_x);
  EXPECT_EQ(alignment::LEFT, blk.align);
  EXPECT_EQ(mousebtn::LEFT, blk.button);
  EXPECT_EQ("cmd", blk.cmd);
}

TEST_F(DispatchTest, actionOffsetEnd) {
  bar_settings settings;
  m_dispatch->parse(settings, r, "%{l}a%{A1:cmd:}bar%{O-3}%{A}");

  const auto& actions = m_action_ctxt->get_blocks();

  ASSERT_EQ(1, actions.size());

  const auto& blk = actions[0];

  EXPECT_EQ(1, blk.start_x);
  EXPECT_EQ(4, blk.end_x);
  EXPECT_EQ(alignment::LEFT, blk.align);
  EXPECT_EQ(mousebtn::LEFT, blk.button);
  EXPECT_EQ("cmd", blk.cmd);
}

TEST_F(DispatchTest, actionOffsetBefore) {
  bar_settings settings;
  m_dispatch->parse(settings, r, "%{l}%{O100 O-100}a%{A1:cmd:}bar%{O-3}%{A}");

  const auto& actions = m_action_ctxt->get_blocks();

  ASSERT_EQ(1, actions.size());

  const auto& blk = actions[0];

  EXPECT_EQ(1, blk.start_x);
  EXPECT_EQ(4, blk.end_x);
  EXPECT_EQ(alignment::LEFT, blk.align);
  EXPECT_EQ(mousebtn::LEFT, blk.button);
  EXPECT_EQ("cmd", blk.cmd);
}
