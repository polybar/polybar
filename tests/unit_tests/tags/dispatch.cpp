#include "tags/dispatch.hpp"

#include "common/test.hpp"
#include "components/logger.hpp"
#include "gmock/gmock.h"

using namespace polybar;
using namespace std;
using namespace tags;

using ::testing::_;
using ::testing::InSequence;

class MockRenderer : public renderer_interface {
 public:
  MockRenderer(action_context& action_ctxt) : renderer_interface(action_ctxt){};

  MOCK_METHOD(void, render_offset, (const context& ctxt, int pixels), (override));
  MOCK_METHOD(void, render_text, (const context& ctxt, const string&& str), (override));
  MOCK_METHOD(void, change_alignment, (const context& ctxt), (override));
  MOCK_METHOD(double, get_x, (const context& ctxt), (const, override));
  MOCK_METHOD(double, get_alignment_start, (const alignment align), (const, override));
};

class TestableDispatch : public dispatch {};

class DispatchTest : public ::testing::Test {
 protected:
  unique_ptr<action_context> m_action_ctxt = make_unique<action_context>();

  unique_ptr<dispatch> m_dispatch = make_unique<dispatch>(logger(loglevel::NONE), *m_action_ctxt);
};

TEST_F(DispatchTest, ignoreFormatting) {
  MockRenderer r(*m_action_ctxt);

  {
    InSequence seq;
    EXPECT_CALL(r, render_offset(_, 10)).Times(1);
    EXPECT_CALL(r, render_text(_, string{"abc"})).Times(1);
    EXPECT_CALL(r, render_text(_, string{"foo"})).Times(1);
  }

  bar_settings settings;

  m_dispatch->parse(settings, r, "%{O10}abc%{F-}foo");
}
