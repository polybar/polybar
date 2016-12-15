#include "components/command_line.cpp"
#include "utils/string.cpp"

int main() {
  using namespace polybar;

  const auto get_opts = []() -> const command_line::options {
    // clang-format off
    return command_line::options{
      command_line::option{"-f", "--flag", "Flag description"},
      command_line::option{"-o", "--option", "Option description", "OPTION", {"foo", "bar", "baz"}},
    };
    // clang-format on
  };

  "has_short"_test = [&] {
    auto cli = cliparser::make("cmd", get_opts());
    cli->process_input(string_util::split("-f", ' '));
    expect(cli->has("flag"));
    expect(!cli->has("option"));

    cli = cliparser::make("cmd", get_opts());;
    cli->process_input(string_util::split("-f -o foo", ' '));
    expect(cli->has("flag"));
    expect(cli->has("option"));

    cli = cliparser::make("cmd", get_opts());;
    cli->process_input(string_util::split("-o baz", ' '));
    expect(!cli->has("flag"));
    expect(cli->has("option"));
  };

  "has_long"_test = [&] {
    auto cli = cliparser::make("cmd", get_opts());;
    cli->process_input(string_util::split("--flag", ' '));
    expect(cli->has("flag"));
    expect(!cli->has("option"));

    cli = cliparser::make("cmd", get_opts());;
    cli->process_input(string_util::split("--flag --option=foo", ' '));
    expect(cli->has("flag"));
    expect(cli->has("option"));

    cli = cliparser::make("cmd", get_opts());;
    cli->process_input(string_util::split("--option=foo --flag", ' '));
    expect(cli->has("flag"));
    expect(cli->has("option"));

    cli = cliparser::make("cmd", get_opts());;
    cli->process_input(string_util::split("--option=baz", ' '));
    expect(!cli->has("flag"));
    expect(cli->has("option"));
  };

  "compare"_test = [&] {
    auto cli = cliparser::make("cmd", get_opts());;
    cli->process_input(string_util::split("-o baz", ' '));
    expect(cli->compare("option", "baz"));

    cli = cliparser::make("cmd", get_opts());;
    cli->process_input(string_util::split("--option=foo", ' '));
    expect(cli->compare("option", "foo"));
  };

  "get"_test = [&] {
    auto cli = cliparser::make("cmd", get_opts());;
    cli->process_input(string_util::split("--option=baz", ' '));
    expect("baz" == cli->get("option"));

    cli = cliparser::make("cmd", get_opts());;
    cli->process_input(string_util::split("--option=foo", ' '));
    expect("foo" == cli->get("option"));
  };

  "missing_value"_test = [&] {
    auto input1 = string_util::split("--option", ' ');
    auto input2 = string_util::split("-o", ' ');
    auto input3 = string_util::split("--option baz", ' ');

    bool exception_thrown = false;
    try {
      auto cli = cliparser::make("cmd", get_opts());;
      cli->process_input(input1);
    } catch (const command_line::value_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);

    exception_thrown = false;  // reset
    try {
      auto cli = cliparser::make("cmd", get_opts());;
      cli->process_input(input2);
    } catch (const command_line::value_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);

    exception_thrown = false;  // reset
    try {
      auto cli = cliparser::make("cmd", get_opts());;
      cli->process_input(input3);
    } catch (const command_line::value_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);
  };

  "invalid_value"_test = [&] {
    auto input1 = string_util::split("--option=invalid", ' ');
    auto input2 = string_util::split("-o invalid_value", ' ');

    bool exception_thrown = false;
    try {
      auto cli = cliparser::make("cmd", get_opts());;
      cli->process_input(input1);
    } catch (const command_line::value_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);

    exception_thrown = false;  // reset
    try {
      auto cli = cliparser::make("cmd", get_opts());;
      cli->process_input(input2);
    } catch (const command_line::value_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);
  };

  "unrecognized"_test = [&] {
    auto input1 = string_util::split("-x", ' ');
    auto input2 = string_util::split("--unrecognized", ' ');

    bool exception_thrown = false;
    try {
      auto cli = cliparser::make("cmd", get_opts());;
      cli->process_input(input1);
    } catch (const command_line::argument_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);

    exception_thrown = false;  // reset
    try {
      auto cli = cliparser::make("cmd", get_opts());;
      cli->process_input(input2);
    } catch (const command_line::argument_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);
  };
}
