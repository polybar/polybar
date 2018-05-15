#include <algorithm>

#include "common/test.hpp"
#include "components/config_parser.hpp"
#include "components/logger.hpp"

using namespace polybar;
using namespace std;

/**
 * \brief Testing-only subclass of config_parser to change access level
 */
class TestableConfigParser : public config_parser {
  using config_parser::config_parser;
  public: using config_parser::get_line_type;
  public: using config_parser::parse_key;
  public: using config_parser::parse_header;
};

/**
 * \brief Fixture class
 */
class ConfigParser : public ::testing::Test {
  protected:
    unique_ptr<TestableConfigParser> parser = make_unique<TestableConfigParser>(logger(loglevel::NONE), "/dev/zero");
};

// ParseLineTest {{{

// }}}

// GetLineTypeTest {{{

/**
 * \brief Class for parameterized tests on get_line_type
 *
 * Parameters are pairs of the expected line type and a string that should be
 * detected as that line type
 */
class GetLineTypeTest :
  public ConfigParser,
  public ::testing::WithParamInterface<pair<line_type, string>> {
  };

/**
 * \brief Helper function generate GetLineTypeTest parameter values
 */
vector<pair<line_type, string>> line_type_transform(vector<string> in, line_type type) {
  vector<pair<line_type, string>> out;

  for (auto i : in) {
    out.emplace_back(type, i);
  }

  return out;
}


/**
 * \brief Parameter values for GetLineTypeTest
 */
auto line_type_key = line_type_transform({"a = b", "  a =b", " a\t =\t \t b", "a = "}, line_type::KEY);
auto line_type_header = line_type_transform({"[section]", " [section]", "[section/sub]"}, line_type::HEADER);
auto line_type_comment = line_type_transform({";abc", "#abc", "\t;abc", " #abc", "\t \t;abc"}, line_type::COMMENT);
auto line_type_empty = line_type_transform({"", " ", "\t" "   \t  \t"}, line_type::EMPTY);
auto line_type_unknown = line_type_transform({"|a", " |a", "a"}, line_type::UNKNOWN);

/**
 * Instantiate GetLineTypeTest for the different line types
 */
INSTANTIATE_TEST_CASE_P(LineTypeKey, GetLineTypeTest, ::testing::ValuesIn(line_type_key),);
INSTANTIATE_TEST_CASE_P(LineTypeHeader, GetLineTypeTest, ::testing::ValuesIn(line_type_header),);
INSTANTIATE_TEST_CASE_P(LineTypeComment, GetLineTypeTest, ::testing::ValuesIn(line_type_comment),);
INSTANTIATE_TEST_CASE_P(LineTypeEmpty, GetLineTypeTest, ::testing::ValuesIn(line_type_empty),);
INSTANTIATE_TEST_CASE_P(LineTypeUnknown, GetLineTypeTest, ::testing::ValuesIn(line_type_unknown),);

/**
 * \brief Parameterized test for get_line_type
 */
TEST_P(GetLineTypeTest, correctness) {
  EXPECT_EQ(GetParam().first, parser->get_line_type(GetParam().second));
}

// }}}

// ParseKeyTest {{{

/**
 * \brief Class for parameterized tests on parse_key
 *
 * The first element of the pair is the expected key-value pair and the second
 * element is the string to be parsed, has to be trimmed and valid.
 */
class ParseKeyTest :
  public ConfigParser,
  public ::testing::WithParamInterface<pair<pair<string, string>, string>> {};


vector<pair<pair<string, string>, string>> parse_key_list = {
  {{"key", "value"}, "key = value"},
  {{"key", "value"}, "key=value"},
  {{"key", "value"}, "key =\"value\""},
  {{"key", "value"}, "key\t=\t \"value\""},
  {{"key", "\"value"}, "key = \"value"},
  {{"key", "value\""}, "key = value\""},
  {{"key", "= value"}, "key == value"},
  {{"key", ""}, "key ="},
  {{"key", ""}, "key =\"\""},
};

INSTANTIATE_TEST_CASE_P(Inst, ParseKeyTest,
    ::testing::ValuesIn(parse_key_list),);

/**
 * Parameterized test for parse_key with valid line
 */
TEST_P(ParseKeyTest, correctness) {
  EXPECT_EQ(GetParam().first, parser->parse_key(GetParam().second));
}

/**
 * Tests if exception is thrown for invalid key line
 */
TEST_F(ParseKeyTest, throwsSyntaxError) {
  EXPECT_THROW(parser->parse_key("forbidden char = value"), syntax_error);
  EXPECT_THROW(parser->parse_key("forbidden\tchar = value"), syntax_error);
}
// }}}

// ParseHeaderTest {{{

/**
 * \brief Class for parameterized tests on parse_key
 *
 * The first element of the pair is the expected key-value pair and the second
 * element is the string to be parsed, has to be trimmed and valid
 */
class ParseHeaderTest :
  public ConfigParser,
  public ::testing::WithParamInterface<pair<string, string>> {};

vector<pair<string, string>> parse_header_list = {
  {"section", "[section]"},
  {"bar/name", "[bar/name]"},
  {"with_underscore", "[with_underscore]"},
};

INSTANTIATE_TEST_CASE_P(Inst, ParseHeaderTest,
    ::testing::ValuesIn(parse_header_list),);

/**
 * Parameterized test for parse_header with valid line
 */
TEST_P(ParseHeaderTest, correctness) {
  EXPECT_EQ(GetParam().first, parser->parse_header(GetParam().second));
}

/**
 * Tests if exception is thrown for invalid header line
 */
TEST_F(ParseHeaderTest, throwsSyntaxError) {
  EXPECT_THROW(parser->parse_header("[no_end"), syntax_error);
  EXPECT_THROW(parser->parse_header("[forbidden char]"), syntax_error);
  EXPECT_THROW(parser->parse_header("[forbidden\tchar]"), syntax_error);
}
// }}}
