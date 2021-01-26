#include "components/command_line.hpp"

#include "common/test.hpp"
#include "utils/string.hpp"

using namespace polybar;

class CommandLine : public ::testing::Test {
 protected:
  virtual void SetUp() override {
    set_cli();
  }

  virtual void set_cli() {
    cli = command_line::parser::make("cmd", get_opts());
  }

  command_line::options get_opts() {
    // clang-format off
      return command_line::options {
        command_line::option{"-f", "--flag", "Flag description"},
        command_line::option{"-o", "--option", "Option description", "OPTION", {"foo", "bar", "baz"}},
      };
    // clang-format on
  };

  command_line::parser::make_type cli;
};

TEST_F(CommandLine, hasShort) {
  cli->process_input(string_util::split("-f", ' '));
  EXPECT_TRUE(cli->has("flag"));
  EXPECT_FALSE(cli->has("option"));

  set_cli();
  cli->process_input(string_util::split("-f -o foo", ' '));
  EXPECT_TRUE(cli->has("flag"));
  EXPECT_TRUE(cli->has("option"));

  set_cli();
  cli->process_input(string_util::split("-o baz", ' '));
  EXPECT_FALSE(cli->has("flag"));
  EXPECT_TRUE(cli->has("option"));
}

TEST_F(CommandLine, hasLong) {
  cli->process_input(string_util::split("--flag", ' '));
  EXPECT_TRUE(cli->has("flag"));
  EXPECT_FALSE(cli->has("option"));

  set_cli();
  cli->process_input(string_util::split("--flag --option=foo", ' '));
  EXPECT_TRUE(cli->has("flag"));
  EXPECT_TRUE(cli->has("option"));

  set_cli();
  cli->process_input(string_util::split("--option=foo --flag", ' '));
  EXPECT_TRUE(cli->has("flag"));
  EXPECT_TRUE(cli->has("option"));

  set_cli();
  cli->process_input(string_util::split("--option=baz", ' '));
  EXPECT_FALSE(cli->has("flag"));
  EXPECT_TRUE(cli->has("option"));
}

TEST_F(CommandLine, compare) {
  cli->process_input(string_util::split("-o baz", ' '));
  EXPECT_TRUE(cli->compare("option", "baz"));

  set_cli();
  cli->process_input(string_util::split("--option=foo", ' '));
  EXPECT_TRUE(cli->compare("option", "foo"));
}

TEST_F(CommandLine, get) {
  cli->process_input(string_util::split("--option=baz", ' '));
  EXPECT_EQ("baz", cli->get("option"));

  set_cli();
  cli->process_input(string_util::split("--option=foo", ' '));
  EXPECT_EQ("foo", cli->get("option"));
}

TEST_F(CommandLine, missingValue) {
  auto input1 = string_util::split("--option", ' ');
  auto input2 = string_util::split("-o", ' ');
  auto input3 = string_util::split("--option baz", ' ');

  EXPECT_THROW(cli->process_input(input1), command_line::value_error);
  set_cli();
  EXPECT_THROW(cli->process_input(input2), command_line::value_error);
  set_cli();
  EXPECT_THROW(cli->process_input(input3), command_line::value_error);
}

TEST_F(CommandLine, invalidValue) {
  auto input1 = string_util::split("--option=invalid", ' ');
  auto input2 = string_util::split("-o invalid_value", ' ');

  EXPECT_THROW(cli->process_input(input1), command_line::value_error);
  set_cli();
  EXPECT_THROW(cli->process_input(input2), command_line::value_error);
}

TEST_F(CommandLine, unrecognized) {
  auto input1 = string_util::split("-x", ' ');
  auto input2 = string_util::split("--unrecognized", ' ');

  EXPECT_THROW(cli->process_input(input1), command_line::argument_error);
  set_cli();
  EXPECT_THROW(cli->process_input(input2), command_line::argument_error);
}
