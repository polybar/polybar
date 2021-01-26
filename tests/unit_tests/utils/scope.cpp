#include "common/test.hpp"
#include "utils/scope.hpp"

using namespace polybar;

TEST(Scope, onExit) {
  auto flag = false;
  {
    EXPECT_FALSE(flag);
    auto handler = scope_util::make_exit_handler<>([&] { flag = true; });
    EXPECT_FALSE(flag);
    {
      auto handler = scope_util::make_exit_handler<>([&] { flag = true; });
    }
    EXPECT_TRUE(flag);
    flag = false;
  }
  EXPECT_TRUE(flag);
}
