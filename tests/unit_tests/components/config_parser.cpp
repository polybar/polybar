#include "components/config_parser.hpp"

#include "common/test.hpp"
#include "components/logger.hpp"

using namespace polybar;
using namespace std;

/**
 * \brief Testing-only subclass of config_parser to change access level
 */
class TestableConfigParser : public config_parser {
  using config_parser::config_parser;

 public:
  TestableConfigParser(const logger& logger, string&& file)
      : config_parser(logger, move(file)) {
    m_files.push_back("test_config");
  }

 public:
  using config_parser::get_line_type;

 public:
  using config_parser::parse_key;

 public:
  using config_parser::parse_header;

 public:
  using config_parser::parse_line;

 public:
  using config_parser::parse_escaped_value;

 public:
  using config_parser::m_files;
};

/**
 * \brief Fixture class
 */
class ConfigParser : public ::testing::Test {
 protected:
  const logger l = logger(loglevel::NONE);
  unique_ptr<TestableConfigParser> parser = make_unique<TestableConfigParser>(l, "/dev/zero");
};

// ParseLineTest {{{
class ParseLineInValidTest : public ConfigParser, public ::testing::WithParamInterface<string> {};

class ParseLineHeaderTest : public ConfigParser, public ::testing::WithParamInterface<pair<string, string>> {};

class ParseLineKeyTest : public ConfigParser,
                         public ::testing::WithParamInterface<pair<pair<string, string>, string>> {};

vector<string> parse_line_invalid_list = {
    " # comment",
    "; comment",
    "\t#",
    "",
    " ",
    "\t ",
};

vector<pair<string, string>> parse_line_header_list = {
    {"section", "\t[section]"},
    {"section", "\t[section]  "},
    {"bar/name", "\t[bar/name]  "},
};

vector<pair<pair<string, string>, string>> parse_line_key_list = {
    {{"key", "value"}, " key = value"},
    {{"key", ""}, " key\t = \"\""},
    {{"key", "\""}, " key\t = \"\"\""},
    {{"key", "\\"}, " key = \\"},
    {{"key", "\\"}, " key = \\\\"},
    {{"key", "\\val\\ue\\"}, " key = \\val\\ue\\"},
    {{"key", "\\val\\ue\\"}, " key = \\\\val\\\\ue\\\\"},
    {{"key", "\\val\\ue\\"}, " key = \"\\val\\ue\\\""},
    {{"key", "\\val\\ue\\"}, " key = \"\\\\val\\\\ue\\\\\""},
};

INSTANTIATE_TEST_SUITE_P(Inst, ParseLineInValidTest, ::testing::ValuesIn(parse_line_invalid_list));

TEST_P(ParseLineInValidTest, correctness) {
  line_t line;
  line.file_index = 0;
  line.line_no = 0;
  parser->parse_line(line, GetParam());

  EXPECT_FALSE(line.useful);
}

INSTANTIATE_TEST_SUITE_P(Inst, ParseLineHeaderTest, ::testing::ValuesIn(parse_line_header_list));

TEST_P(ParseLineHeaderTest, correctness) {
  line_t line;
  line.file_index = 0;
  line.line_no = 0;
  parser->parse_line(line, GetParam().second);

  EXPECT_TRUE(line.useful);

  EXPECT_TRUE(line.is_header);
  EXPECT_EQ(GetParam().first, line.header);
}

INSTANTIATE_TEST_SUITE_P(Inst, ParseLineKeyTest, ::testing::ValuesIn(parse_line_key_list));

TEST_P(ParseLineKeyTest, correctness) {
  line_t line;
  line.file_index = 0;
  line.line_no = 0;
  parser->parse_line(line, GetParam().second);

  EXPECT_TRUE(line.useful);

  EXPECT_FALSE(line.is_header);
  EXPECT_EQ(GetParam().first.first, line.key);
  EXPECT_EQ(GetParam().first.second, line.value);
}

TEST_F(ParseLineInValidTest, throwsSyntaxError) {
  line_t line;
  line.file_index = 0;
  line.line_no = 0;

  EXPECT_THROW(parser->parse_line(line, "unknown"), syntax_error);
  EXPECT_THROW(parser->parse_line(line, "\ufeff"), syntax_error);
}
// }}}

// GetLineTypeTest {{{

/**
 * \brief Class for parameterized tests on get_line_type
 *
 * Parameters are pairs of the expected line type and a string that should be
 * detected as that line type
 */
class GetLineTypeTest : public ConfigParser, public ::testing::WithParamInterface<pair<line_type, string>> {};

/**
 * \brief Helper function generate GetLineTypeTest parameter values
 */
vector<pair<line_type, string>> line_type_transform(vector<string>&& in, line_type type) {
  vector<pair<line_type, string>> out;

  out.reserve(in.size());
  for (const auto& i : in) {
    out.emplace_back(type, i);
  }

  return out;
}

/**
 * \brief Parameter values for GetLineTypeTest
 */
auto line_type_key = line_type_transform({"a = b", "  a =b", " a\t =\t \t b", "a = "}, line_type::KEY);
auto line_type_header = line_type_transform({"[section]", "[section]", "[section/sub]"}, line_type::HEADER);
auto line_type_comment = line_type_transform({";abc", "#abc", ";", "#"}, line_type::COMMENT);
auto line_type_empty = line_type_transform({""}, line_type::EMPTY);
auto line_type_unknown = line_type_transform({"|a", " |a", "a"}, line_type::UNKNOWN);

/**
 * Instantiate GetLineTypeTest for the different line types
 */
INSTANTIATE_TEST_SUITE_P(LineTypeKey, GetLineTypeTest, ::testing::ValuesIn(line_type_key));
INSTANTIATE_TEST_SUITE_P(LineTypeHeader, GetLineTypeTest, ::testing::ValuesIn(line_type_header));
INSTANTIATE_TEST_SUITE_P(LineTypeComment, GetLineTypeTest, ::testing::ValuesIn(line_type_comment));
INSTANTIATE_TEST_SUITE_P(LineTypeEmpty, GetLineTypeTest, ::testing::ValuesIn(line_type_empty));
INSTANTIATE_TEST_SUITE_P(LineTypeUnknown, GetLineTypeTest, ::testing::ValuesIn(line_type_unknown));

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
class ParseKeyTest : public ConfigParser, public ::testing::WithParamInterface<pair<pair<string, string>, string>> {};

vector<pair<pair<string, string>, string>> parse_key_list = {
    {{"key", "value"}, "key = value"},
    {{"key", "value"}, "key=value"},
    {{"key", "value"}, "key =\"value\""},
    {{"key", "value"}, "key\t=\t \"value\""},
    {{"key", "\"value"}, "key = \"value"},
    {{"key", "value\""}, "key = value\""},
    {{"key", "= value"}, "key == value"},
    {{"key", ""}, "key ="},
    {{"key", ""}, R"(key ="")"},
    {{"key", "\"\""}, R"(key ="""")"},
};

INSTANTIATE_TEST_SUITE_P(Inst, ParseKeyTest, ::testing::ValuesIn(parse_key_list));

/**
 * Parameterized test for parse_key with valid line
 */
TEST_P(ParseKeyTest, correctness) {
  line_t line;
  line.file_index = 0;
  line.line_no = 0;
  EXPECT_EQ(GetParam().first, parser->parse_key(line, GetParam().second));
}

/**
 * Tests if exception is thrown for invalid key line
 */
TEST_F(ParseKeyTest, throwsSyntaxError) {
  line_t line;
  line.file_index = 0;
  line.line_no = 0;

  EXPECT_THROW(parser->parse_key(line, "= empty name"), syntax_error);
  EXPECT_THROW(parser->parse_key(line, "forbidden char = value"), syntax_error);
  EXPECT_THROW(parser->parse_key(line, "forbidden\tchar = value"), syntax_error);
}
// }}}

// ParseHeaderTest {{{

/**
 * \brief Class for parameterized tests on parse_key
 *
 * The first element of the pair is the expected key-value pair and the second
 * element is the string to be parsed, has to be trimmed and valid
 */
class ParseHeaderTest : public ConfigParser, public ::testing::WithParamInterface<pair<string, string>> {};

vector<pair<string, string>> parse_header_list = {
    {"section", "[section]"},
    {"bar/name", "[bar/name]"},
    {"with_underscore", "[with_underscore]"},
};

INSTANTIATE_TEST_SUITE_P(Inst, ParseHeaderTest, ::testing::ValuesIn(parse_header_list));

/**
 * Parameterized test for parse_header with valid line
 */
TEST_P(ParseHeaderTest, correctness) {
  line_t line;
  line.file_index = 0;
  line.line_no = 0;
  EXPECT_EQ(GetParam().first, parser->parse_header(line, GetParam().second));
}

/**
 * Tests if exception is thrown for invalid header line
 */
TEST_F(ParseHeaderTest, throwsSyntaxError) {
  line_t line;
  line.file_index = 0;
  line.line_no = 0;

  EXPECT_THROW(parser->parse_header(line, "[]"), syntax_error);
  EXPECT_THROW(parser->parse_header(line, "[no_end"), syntax_error);
  EXPECT_THROW(parser->parse_header(line, "[forbidden char]"), syntax_error);
  EXPECT_THROW(parser->parse_header(line, "[forbidden\tchar]"), syntax_error);

  // Reserved names
  EXPECT_THROW(parser->parse_header(line, "[self]"), syntax_error);
  EXPECT_THROW(parser->parse_header(line, "[BAR]"), syntax_error);
  EXPECT_THROW(parser->parse_header(line, "[root]"), syntax_error);
}
// }}}

// ParseEscapedValueTest {{{

/**
 * \brief Class for parameterized tests on parse_escaped_value
 *
 * The first element of the pair is the expected value and the second
 * element is the escaped string to be parsed.
 */
class ParseEscapedValueTest : public ConfigParser, public ::testing::WithParamInterface<pair<string, string>> {};

vector<pair<string, string>> parse_escaped_value_list = {
    {"\\", "\\"},
    {"\\", "\\\\"},
    {"\\val\\ue\\", "\\val\\ue\\"},
    {"\\val\\ue\\", "\\\\val\\\\ue\\\\"},
    {"\"\\val\\ue\\\"", "\"\\val\\ue\\\""},
    {"\"\\val\\ue\\\"", "\"\\\\val\\\\ue\\\\\""},
};

INSTANTIATE_TEST_SUITE_P(Inst, ParseEscapedValueTest, ::testing::ValuesIn(parse_escaped_value_list));

/**
 * Parameterized test for parse_escaped_value with valid line
 */
TEST_P(ParseEscapedValueTest, correctness) {
  line_t line;
  line.file_index = 0;
  line.line_no = 0;
  string value = GetParam().second;
  value = parser->parse_escaped_value(line, move(value), "key");
  EXPECT_EQ(GetParam().first, value);
}
// }}}
