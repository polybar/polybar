#include "utils/file.hpp"

#include <iomanip>
#include <iostream>

#include "common/test.hpp"
#include "utils/command.hpp"

using namespace polybar;

TEST(File, expand) {
  auto cmd = command_util::make_command<output_policy::REDIRECTED>("echo $HOME");
  cmd->exec();
  cmd->tail([](string home) { EXPECT_EQ(home + "/test", file_util::expand("~/test")); });
}

TEST(File, expand_relative) {
  EXPECT_EQ(file_util::expand("../test", "/scratch/polybar"), "/scratch/polybar/../test");
  EXPECT_EQ(file_util::expand("modules/battery", "/scratch/polybar"), "/scratch/polybar/modules/battery");
  EXPECT_EQ(file_util::expand("/tmp/foo", "/scratch"), "/tmp/foo");
}
