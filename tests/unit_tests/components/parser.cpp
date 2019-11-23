#include "common/test.hpp"

#include "components/parser.hpp"

#include <deque>

#include "components/types.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "events/signal_receiver.hpp"

using namespace polybar;

class TestableParser : public parser {
  using parser::parser;

 public:
  using parser::parse_action_cmd;
};

class Parser : public ::testing::Test {
 protected:
  TestableParser m_parser{signal_emitter::make()};
};

enum class event_type { OFFSET, TEXT };

class FakeSignalReceiver
    : public signal_receiver<SIGN_PRIORITY_RENDERER, signals::parser::offset_pixel, signals::parser::text> {
 public:
  explicit FakeSignalReceiver(std::deque<pair<event_type, string>>&& sequence)
      : em{signal_emitter::make()}, sequence(move(sequence)) {
    em.attach(this);
  }

  ~FakeSignalReceiver() override {
    em.detach(this);
  }

  bool on(const signals::parser::offset_pixel& evt) override {
    if (sequence.empty()) {
      valid = false;
      return false;
    }

    auto str = to_string(evt.cast());
    auto expected_result = sequence.front();
    sequence.pop_front();

    valid &= expected_result.first == event_type::OFFSET && str == expected_result.second;
    return true;
  }

  bool on(const signals::parser::text& evt) override {
    if (sequence.empty()) {
      valid = false;
      return true;
    }

    auto str = evt.cast();
    auto expected_result = sequence.front();
    sequence.pop_front();

    valid &= expected_result.first == event_type::TEXT && str == expected_result.second;
    return true;
  }

  bool valid = true;
  signal_emitter em;
  std::deque<pair<event_type, string>> sequence;
};

class ParserWithSignalHolder : public ::testing::Test {
 protected:
  TestableParser m_parser{signal_emitter::make()};
};
/**
 * The first element of the pair is the expected return text, the second element
 * is the input to parse_action_cmd
 */
class ParseActionCmd : public Parser, public ::testing::WithParamInterface<pair<string, string>> {};

vector<pair<string, string>> parse_action_cmd_list = {
    {"abc", ":abc:\\abc"},
    {"abc\\:", ":abc\\::\\abc"},
    {"\\:\\:\\:", ":\\:\\:\\::\\abc"},
};

INSTANTIATE_TEST_SUITE_P(Inst, ParseActionCmd, ::testing::ValuesIn(parse_action_cmd_list));

TEST_P(ParseActionCmd, correctness) {
  auto input = GetParam().second;
  auto result = m_parser.parse_action_cmd(std::move(input));
  EXPECT_EQ(GetParam().first, result);
}

class ParseData : public Parser,
                  public ::testing::WithParamInterface<pair<string, std::deque<pair<event_type, string>>>> {};

// clang-format off
vector<pair<string, std::deque<pair<event_type, string>>>> parse_data = {
    {"Test\\%{O100}", std::deque<pair<event_type, string>>{{event_type::TEXT, "Test"}, {event_type::TEXT, "%{O100}"}}},
    {"Test\\\\%{O100}", std::deque<pair<event_type, string>>{{event_type::TEXT, "Test\\"}, {event_type::TEXT, "%{O100}"}}},
    {"Test%{O100}", std::deque<pair<event_type, string>>{{event_type::TEXT, "Test"}, {event_type::OFFSET, "100"}}},
    {"Test %{O100}", std::deque<pair<event_type, string>>{{event_type::TEXT, "Test "}, {event_type::OFFSET, "100"}}},
    {"Test \\%{O100}", std::deque<pair<event_type, string>>{{event_type::TEXT, "Test "}, {event_type::TEXT, "%{O100}"}}},
    {"Test\\ %{O100}", std::deque<pair<event_type, string>>{{event_type::TEXT, "Test\\ "}, {event_type::OFFSET, "100"}}},
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(Inst, ParseData, ::testing::ValuesIn(parse_data));

TEST_P(ParseData, correctness) {
  string input = GetParam().first;
  std::deque<pair<event_type, string>> sequence = GetParam().second;

  FakeSignalReceiver signalHolder(move(sequence));
  bar_settings b{};
  m_parser.parse(b, std::move(input));

  ASSERT_TRUE(signalHolder.valid);
}
