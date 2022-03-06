#include "utils/scope.hpp"

#include "common/test.hpp"

using namespace polybar;

TEST(Scope, onExit) {
  auto flag = false;
  {
    EXPECT_FALSE(flag);
    scope_util::on_exit handler([&] { flag = true; });
    EXPECT_FALSE(flag);
    {
      scope_util::on_exit handler([&] { flag = true; });
    }
    EXPECT_TRUE(flag);
    flag = false;
  }
  EXPECT_TRUE(flag);
}
