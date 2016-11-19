#include "components/command_line.hpp"
#include "utils/string.hpp"

int main() {
  using namespace polybar;

  // clang-format off
  const command_line::options opts{
    command_line::option{"-f", "--flag", "Flag description"},
    command_line::option{"-o", "--option", "Option description", "OPTION", {"foo", "bar", "baz"}},
  };
  // clang-format on

  "has_short"_test = [&opts] {
    auto cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
    cli.process_input(string_util::split("-f", ' '));
    expect(cli.has("flag"));
    expect(!cli.has("option"));

    cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
    cli.process_input(string_util::split("-f -o foo", ' '));
    expect(cli.has("flag"));
    expect(cli.has("option"));

    cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
    cli.process_input(string_util::split("-o baz", ' '));
    expect(!cli.has("flag"));
    expect(cli.has("option"));
  };

  "has_long"_test = [&opts] {
    auto cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
    cli.process_input(string_util::split("--flag", ' '));
    expect(cli.has("flag"));
    expect(!cli.has("option"));

    cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
    cli.process_input(string_util::split("--flag --option=foo", ' '));
    expect(cli.has("flag"));
    expect(cli.has("option"));

    cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
    cli.process_input(string_util::split("--option=foo --flag", ' '));
    expect(cli.has("flag"));
    expect(cli.has("option"));

    cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
    cli.process_input(string_util::split("--option=baz", ' '));
    expect(!cli.has("flag"));
    expect(cli.has("option"));
  };

  "compare"_test = [&opts] {
    auto cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
    cli.process_input(string_util::split("-o baz", ' '));
    expect(cli.compare("option", "baz"));

    cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
    cli.process_input(string_util::split("--option=foo", ' '));
    expect(cli.compare("option", "foo"));
  };

  "get"_test = [&opts] {
    auto cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
    cli.process_input(string_util::split("--option=baz", ' '));
    expect("baz" == cli.get("option"));

    cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
    cli.process_input(string_util::split("--option=foo", ' '));
    expect("foo" == cli.get("option"));
  };

  "missing_value"_test = [&opts] {
    auto input1 = string_util::split("--option", ' ');
    auto input2 = string_util::split("-o", ' ');
    auto input3 = string_util::split("--option baz", ' ');

    bool exception_thrown = false;
    try {
      auto cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
      cli.process_input(input1);
    } catch (const command_line::value_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);

    exception_thrown = false;  // reset
    try {
      auto cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
      cli.process_input(input2);
    } catch (const command_line::value_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);

    exception_thrown = false;  // reset
    try {
      auto cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
      cli.process_input(input3);
    } catch (const command_line::value_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);
  };

  "invalid_value"_test = [&opts] {
    auto input1 = string_util::split("--option=invalid", ' ');
    auto input2 = string_util::split("-o invalid_value", ' ');

    bool exception_thrown = false;
    try {
      auto cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
      cli.process_input(input1);
    } catch (const command_line::value_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);

    exception_thrown = false;  // reset
    try {
      auto cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
      cli.process_input(input2);
    } catch (const command_line::value_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);
  };

  "unrecognized"_test = [&opts] {
    auto input1 = string_util::split("-x", ' ');
    auto input2 = string_util::split("--unrecognized", ' ');

    bool exception_thrown = false;
    try {
      auto cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
      cli.process_input(input1);
    } catch (const command_line::argument_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);

    exception_thrown = false;  // reset
    try {
      auto cli = configure_cliparser<cliparser>("cmd", opts).create<cliparser>();
      cli.process_input(input2);
    } catch (const command_line::argument_error&) {
      exception_thrown = true;
    } catch (...) {
    }
    expect(exception_thrown);
  };
}
