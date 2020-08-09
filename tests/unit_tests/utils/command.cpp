#include "utils/command.hpp"
#include "common/test.hpp"

#include <unistd.h>

using namespace polybar;

TEST(Command, status) {
  // Test for command<output_policy::IGNORED>::exec(bool);
  {
    auto cmd = command_util::make_command<output_policy::IGNORED>("echo polybar > /dev/null");
    int status = cmd->exec();

    EXPECT_EQ(status, EXIT_SUCCESS);
  }

  // Test for command<output_policy::REDIRECTED>::exec(bool);
  {
    auto cmd = command_util::make_command<output_policy::REDIRECTED>("echo polybar");
    int status = cmd->exec();

    EXPECT_EQ(status, EXIT_SUCCESS);
  }
}

TEST(Command, status_async) {
  {
    auto cmd = command_util::make_command<output_policy::IGNORED>("echo polybar > /dev/null");
    EXPECT_EQ(cmd->exec(false), EXIT_SUCCESS);

    cmd->wait();

    EXPECT_FALSE(cmd->is_running());
    EXPECT_EQ(cmd->get_exit_status(), EXIT_SUCCESS);
  }
}

TEST(Command, output) {
  auto cmd = command_util::make_command<output_policy::REDIRECTED>("echo polybar");
  string str;
  cmd->exec(false);
  cmd->tail([&str](string&& string) { str = string; });
  cmd->wait();

  EXPECT_EQ(str, "polybar");
}

TEST(Command, readline) {
  auto cmd = command_util::make_command<output_policy::REDIRECTED>("read text;echo $text");

  string str;
  cmd->exec(false);
  cmd->writeline("polybar");
  cmd->tail([&str](string&& string) { str = string; });

  EXPECT_EQ(str, "polybar");
}
