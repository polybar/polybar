#include <lemonbuddy/components/command_line.hpp>
#include <lemonbuddy/utils/string.hpp>

#include "../unit_test.hpp"

using namespace lemonbuddy;
using cli_parser = command_line::parser;

// clang-format off
const command_line::options g_opts{
  command_line::option{"-f", "--flag", "Flag description"},
  command_line::option{"-o", "--option", "Option description", "OPTION", {"foo", "bar", "baz"}},
};
// clang-format on

class test_command_line : public unit_test {
 public:
  CPPUNIT_TEST_SUITE(test_command_line);
  CPPUNIT_TEST(test_has_short);
  CPPUNIT_TEST(test_has_long);
  CPPUNIT_TEST(test_compare);
  CPPUNIT_TEST(test_get);
  CPPUNIT_TEST(test_missing_value);
  CPPUNIT_TEST(test_invalid_value);
  CPPUNIT_TEST(test_unrecognized);
  CPPUNIT_TEST_SUITE_END();

  void test_has_short() {
    auto cli = get_instance();
    cli.process_input(string_util::split("-f", ' '));
    CPPUNIT_ASSERT_EQUAL(true, cli.has("flag"));
    CPPUNIT_ASSERT_EQUAL(false, cli.has("option"));

    cli = get_instance();
    cli.process_input(string_util::split("-f -o foo", ' '));
    CPPUNIT_ASSERT_EQUAL(true, cli.has("flag"));
    CPPUNIT_ASSERT_EQUAL(true, cli.has("option"));

    cli = get_instance();
    cli.process_input(string_util::split("-o baz", ' '));
    CPPUNIT_ASSERT_EQUAL(false, cli.has("flag"));
    CPPUNIT_ASSERT_EQUAL(true, cli.has("option"));
  }

  void test_has_long() {
    auto cli = get_instance();
    cli.process_input(string_util::split("--flag", ' '));
    CPPUNIT_ASSERT_EQUAL(true, cli.has("flag"));
    CPPUNIT_ASSERT_EQUAL(false, cli.has("option"));

    cli = get_instance();
    cli.process_input(string_util::split("--flag --option=foo", ' '));
    CPPUNIT_ASSERT_EQUAL(true, cli.has("flag"));
    CPPUNIT_ASSERT_EQUAL(true, cli.has("option"));

    cli = get_instance();
    cli.process_input(string_util::split("--option=foo --flag", ' '));
    CPPUNIT_ASSERT_EQUAL(true, cli.has("flag"));
    CPPUNIT_ASSERT_EQUAL(true, cli.has("option"));

    cli = get_instance();
    cli.process_input(string_util::split("--option=baz", ' '));
    CPPUNIT_ASSERT_EQUAL(false, cli.has("flag"));
    CPPUNIT_ASSERT_EQUAL(true, cli.has("option"));
  }

  void test_compare() {
    auto cli = get_instance();
    cli.process_input(string_util::split("-o baz", ' '));
    CPPUNIT_ASSERT_EQUAL(true, cli.compare("option", "baz"));

    cli = get_instance();
    cli.process_input(string_util::split("--option=foo", ' '));
    CPPUNIT_ASSERT_EQUAL(true, cli.compare("option", "foo"));
  }

  void test_get() {
    auto cli = get_instance();
    cli.process_input(string_util::split("--option=baz", ' '));
    CPPUNIT_ASSERT_EQUAL(string{"baz"}, cli.get("option"));

    cli = get_instance();
    cli.process_input(string_util::split("--option=foo", ' '));
    CPPUNIT_ASSERT_EQUAL(string{"foo"}, cli.get("option"));
  }

  void test_missing_value() {
    auto input1 = string_util::split("--option", ' ');
    auto input2 = string_util::split("-o", ' ');
    auto input3 = string_util::split("--option baz", ' ');

    using command_line::value_error;

    CPPUNIT_ASSERT_THROW(get_instance().process_input(input1), value_error);
    CPPUNIT_ASSERT_THROW(get_instance().process_input(input2), value_error);
    CPPUNIT_ASSERT_THROW(get_instance().process_input(input3), value_error);
  }

  void test_invalid_value() {
    auto input1 = string_util::split("--option=invalid", ' ');
    auto input2 = string_util::split("-o invalid_value", ' ');

    using command_line::value_error;

    CPPUNIT_ASSERT_THROW(get_instance().process_input(input1), value_error);
    CPPUNIT_ASSERT_THROW(get_instance().process_input(input2), value_error);
  }

  void test_unrecognized() {
    auto input1 = string_util::split("-x", ' ');
    auto input2 = string_util::split("--unrecognized", ' ');

    using command_line::argument_error;

    CPPUNIT_ASSERT_THROW(get_instance().process_input(input1), argument_error);
    CPPUNIT_ASSERT_THROW(get_instance().process_input(input2), argument_error);
  }

 private:
  cli_parser get_instance() {
    return cli_parser::configure<cli_parser>("cmd", g_opts).create<cli_parser>();
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(test_command_line);
