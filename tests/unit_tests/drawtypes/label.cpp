#include "common/test.hpp"

#include <tuple>
#include <vector>

#include "drawtypes/label.hpp"

using namespace polybar;

class Label : public ::testing::Test, public ::testing::WithParamInterface<std::tuple<string, bool, string, string>> {
 public:
  static const string token_str;
  const drawtypes::token token{token_str};
};

const string Label::token_str = "%percentage%";

vector<std::tuple<string, bool, string, string>> values{
    std::make_tuple(Label::token_str, false, "97", "97"),
    std::make_tuple(Label::token_str, false, "97%", "97%"),
    std::make_tuple(Label::token_str, true, "97", "97"),
    std::make_tuple(Label::token_str, true, "97%", "97%"),
    std::make_tuple(Label::token_str, false, "Test%{O100}", "Test\\%{O100}"),
    std::make_tuple(Label::token_str, true, "Test%{O100}", "Test%{O100}"),
    std::make_tuple(Label::token_str, true, "Test%%{O100}", "Test%%{O100}"),
    std::make_tuple(Label::token_str, false, "Test%%{O100}", "Test%\\%{O100}"),
    std::make_tuple(Label::token_str, false, "Test\\", "Test\\"),
    std::make_tuple(Label::token_str, true, "Test\\", "Test\\"),
    std::make_tuple(Label::token_str, false, "Test\\%{O100}", "Test\\\\%{O100}"),
    std::make_tuple(Label::token_str, true, "Test\\%{O100}", "Test\\%{O100}"),
    std::make_tuple(Label::token_str, false, "\\", "\\"),
    std::make_tuple(Label::token_str, true, "\\", "\\"),
    std::make_tuple(Label::token_str, false, "%", "%"),
    std::make_tuple(Label::token_str, true, "%", "%")
};

INSTANTIATE_TEST_SUITE_P(NoOffset, Label, ::testing::ValuesIn(values));

TEST_P(Label, correctness) {
  string input_text;
  bool whitelisted;
  string replacement_str;
  string output_text;

  std::tie(input_text, whitelisted, replacement_str, output_text) = GetParam();

  drawtypes::label_t l;

  if (whitelisted) {
    l = make_shared<drawtypes::label>(input_text, "", "", "", "", 0, side_values{0U, 0U}, side_values{0U, 0U}, 0_z,
        true, vector<drawtypes::token>{token}, vector<string>{Label::token_str});
  } else {
    l = make_shared<drawtypes::label>(input_text, "", "", "", "", 0, side_values{0U, 0U}, side_values{0U, 0U}, 0_z,
        true, vector<drawtypes::token>{token}, vector<string>{});
  }

  l->replace_token(Label::token_str, replacement_str);

  string result = l->get();

  ASSERT_EQ(result, output_text);
}
