#include "utils/command.hpp"

#include <unistd.h>

#include "common/test.hpp"

using namespace polybar;

static logger null_logger(loglevel::NONE);

TEST(Command, status) {
  // Test for command<output_policy::IGNORED>::exec(bool);
  {
    command<output_policy::IGNORED> cmd(null_logger, "echo polybar > /dev/null");
    int status = cmd.exec();

    EXPECT_EQ(status, EXIT_SUCCESS);
  }

  // Test for command<output_policy::REDIRECTED>::exec(bool);
  {
    command<output_policy::REDIRECTED> cmd(null_logger, "echo polybar");
    int status = cmd.exec();

    EXPECT_EQ(status, EXIT_SUCCESS);
  }
}

TEST(Command, status_async) {
  {
    command<output_policy::IGNORED> cmd(null_logger, "echo polybar > /dev/null");
    EXPECT_EQ(cmd.exec(false), EXIT_SUCCESS);

    cmd.wait();

    EXPECT_FALSE(cmd.is_running());
    EXPECT_EQ(cmd.get_exit_status(), EXIT_SUCCESS);
  }
}

TEST(Command, output) {
  command<output_policy::REDIRECTED> cmd(null_logger, "echo polybar");
  string str;
  cmd.exec(false);
  cmd.tail([&str](string&& string) { str = string; });
  cmd.wait();

  EXPECT_EQ(str, "polybar");
}
