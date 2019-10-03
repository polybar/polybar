#include "common/test.hpp"
#include "events/signal_emitter.hpp"
#include "components/parser.hpp"

using namespace polybar;

class TestableParser : public parser {
  using parser::parser;
  public: using parser::parse_action_cmd;
};

class Parser : public ::testing::Test {
  protected:
    TestableParser m_parser{signal_emitter::make()};
};
/**
 * The first element of the pair is the expected return text, the second element
 * is the input to parse_action_cmd
 */
class ParseActionCmd :
  public Parser,
  public ::testing::WithParamInterface<pair<string, string>> {};

vector<pair<string, string>> parse_action_cmd_list = {
  {"abc", ":abc:\\abc"},
  {"abc\\:", ":abc\\::\\abc"},
  {"\\:\\:\\:", ":\\:\\:\\::\\abc"},
};

INSTANTIATE_TEST_SUITE_P(Inst, ParseActionCmd,
    ::testing::ValuesIn(parse_action_cmd_list));

TEST_P(ParseActionCmd, correctness) {
  auto input = GetParam().second;
  auto result = m_parser.parse_action_cmd(std::move(input));
  EXPECT_EQ(GetParam().first, result);
}
