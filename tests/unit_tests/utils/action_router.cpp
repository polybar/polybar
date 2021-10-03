#include "utils/action_router.hpp"

#include "common/test.hpp"
#include "gmock/gmock.h"

using namespace polybar;
using ::testing::InSequence;

class MockModule {
 public:
  MOCK_METHOD(void, action1, ());
  MOCK_METHOD(void, action2, (const string&));
};

TEST(ActionRouterTest, CallsCorrectFunctions) {
  MockModule m;

  {
    InSequence seq;
    EXPECT_CALL(m, action1()).Times(1);
    EXPECT_CALL(m, action2("foo")).Times(1);
  }

  action_router router;
  router.register_action("action1", [&]() { m.action1(); });
  router.register_action_with_data("action2", [&](const std::string& data) { m.action2(data); });
  router.invoke("action1", "");
  router.invoke("action2", "foo");
}

TEST(ActionRouterTest, HasAction) {
  MockModule m;
  action_router router;

  router.register_action("foo", [&]() { m.action1(); });

  EXPECT_TRUE(router.has_action("foo"));
  EXPECT_FALSE(router.has_action("bar"));
}

TEST(ActionRouterTest, ThrowsOnDuplicate) {
  MockModule m;
  action_router router;

  router.register_action("foo", [&]() { m.action1(); });
  EXPECT_THROW(router.register_action("foo", [&]() { m.action1(); }), std::invalid_argument);
  EXPECT_THROW(router.register_action_with_data("foo", [&](const std::string& data) { m.action2(data); }),
      std::invalid_argument);
}
