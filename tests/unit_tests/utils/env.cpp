#include "utils/env.hpp"

#include "common/test.hpp"
#include "stdlib.h"

using namespace polybar;

static constexpr auto INEXISTENT_ENV = "POLYBAR_INEXISTENT";

TEST(Env, has) {
  EXPECT_EQ(true, env_util::has("HOME"));
  unsetenv(INEXISTENT_ENV);
  EXPECT_EQ(false, env_util::has(INEXISTENT_ENV));
  setenv(INEXISTENT_ENV, "123", false);
  EXPECT_EQ(true, env_util::has(INEXISTENT_ENV));
  unsetenv(INEXISTENT_ENV);
  EXPECT_EQ(false, env_util::has(INEXISTENT_ENV));
}

TEST(Env, get) {
  unsetenv(INEXISTENT_ENV);
  EXPECT_EQ("fallback", env_util::get(INEXISTENT_ENV, "fallback"));
  setenv(INEXISTENT_ENV, "123", false);
  EXPECT_EQ("123", env_util::get(INEXISTENT_ENV, "fallback"));
  unsetenv(INEXISTENT_ENV);
  EXPECT_EQ("fallback", env_util::get(INEXISTENT_ENV, "fallback"));
}
