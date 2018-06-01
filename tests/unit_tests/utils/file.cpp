#include <iomanip>
#include <iostream>

#include "common/test.hpp"
#include "utils/command.hpp"
#include "utils/file.hpp"

using namespace polybar;

TEST(File, expand) {
  auto cmd = command_util::make_command("echo $HOME");
  cmd->exec();
  cmd->tail([](string home) {
      EXPECT_EQ(home + "/test", file_util::expand("~/test"));
      });
}

