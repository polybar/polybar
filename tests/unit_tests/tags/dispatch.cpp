#include "tags/dispatch.hpp"

#include "common/test.hpp"
#include "components/logger.hpp"
#include "gmock/gmock.h"

using namespace polybar;
using namespace std;
using namespace tags;

using ::testing::InSequence;
using ::testing::_;

class MockRenderer : public renderer_interface {
 public:
  MockRenderer(action_context& action_ctxt) : renderer_interface(action_ctxt){};

  MOCK_METHOD(void, render_offset, (const context& ctxt, int pixels), (override));
  MOCK_METHOD(void, render_text, (const context& ctxt, const string&& str), (override));
  MOCK_METHOD(void, change_alignment, (const context& ctxt), (override));
  MOCK_METHOD(void, action_open, (const context& ctxt, action_t id), (override));
  MOCK_METHOD(void, action_close, (const context& ctxt, action_t id), (override));
};

class TestableDispatch : public dispatch {};

class Dispatch : public ::testing::Test {
 protected:
  unique_ptr<action_context> action_ctxt = make_unique<action_context>();

  unique_ptr<dispatch> parser = make_unique<dispatch>(logger(loglevel::NONE), *action_ctxt);
};

TEST_F(Dispatch, ignoreFormatting) {
  MockRenderer r(*action_ctxt);

  {
    InSequence seq;
    EXPECT_CALL(r, render_offset(_, 10)).Times(1);
    EXPECT_CALL(r, render_text(_, string{"abc"})).Times(1);
    EXPECT_CALL(r, render_text(_, string{"foo"})).Times(1);
  }

  bar_settings settings;

  parser->parse(settings, r, "%{O10}abc%{F-}foo");
}
