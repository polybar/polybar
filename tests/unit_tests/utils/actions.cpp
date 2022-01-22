#include "utils/actions.hpp"

#include "common/test.hpp"

using namespace polybar;
using namespace actions_util;

template <typename T1, typename T2, typename T3>
using triple = std::tuple<T1, T2, T3>;

class GetActionStringTest : public ::testing::TestWithParam<pair<triple<string, string, string>, string>> {};

vector<pair<triple<string, string, string>, string>> get_action_string_list = {
    {{"foo", "bar", ""}, "#foo.bar"},
    {{"foo", "bar", "data"}, "#foo.bar.data"},
    {{"foo", "bar", "data.data2"}, "#foo.bar.data.data2"},
    {{"a", "b", "c"}, "#a.b.c"},
    {{"a", "b", ""}, "#a.b"},
};

TEST_P(GetActionStringTest, correctness) {
  auto action = GetParam().first;
  auto exp = GetParam().second;

  auto res = get_action_string(std::get<0>(action), std::get<1>(action), std::get<2>(action));

  EXPECT_EQ(res, exp);
}

INSTANTIATE_TEST_SUITE_P(Inst, GetActionStringTest, ::testing::ValuesIn(get_action_string_list));

class ParseActionStringTest : public ::testing::TestWithParam<pair<string, triple<string, string, string>>> {};

vector<pair<string, triple<string, string, string>>> parse_action_string_list = {
    {"#foo.bar", {"foo", "bar", ""}},
    {"#foo.bar.", {"foo", "bar", ""}},
    {"#foo.bar.data", {"foo", "bar", "data"}},
    {"#foo.bar.data.data2", {"foo", "bar", "data.data2"}},
    {"#a.b.c", {"a", "b", "c"}},
    {"#a.b.", {"a", "b", ""}},
    {"#a.b", {"a", "b", ""}},
};

TEST_P(ParseActionStringTest, correctness) {
  auto action_string = GetParam().first;
  auto exp = GetParam().second;

  auto res = parse_action_string(action_string);

  EXPECT_EQ(res, exp);
}

INSTANTIATE_TEST_SUITE_P(Inst, ParseActionStringTest, ::testing::ValuesIn(parse_action_string_list));

class ParseActionStringThrowTest : public ::testing::TestWithParam<string> {};

vector<string> parse_action_string_throw_list = {
    "#",
    "#.",
    "#..",
    "#handler..",
    "#.action.",
    "#.action.data",
    "#..data",
    "#.data",
};

INSTANTIATE_TEST_SUITE_P(Inst, ParseActionStringThrowTest, ::testing::ValuesIn(parse_action_string_throw_list));

TEST_P(ParseActionStringThrowTest, correctness) {
  EXPECT_THROW(parse_action_string(GetParam()), std::runtime_error);
}
