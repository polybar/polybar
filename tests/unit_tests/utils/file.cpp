#include "utils/file.hpp"

#include <iomanip>
#include <iostream>

#include "common/test.hpp"
#include "utils/command.hpp"
#include "utils/env.hpp"

using namespace polybar;

using expand_test_t = pair<string, string>;
class ExpandTest : public testing::TestWithParam<expand_test_t> {};

vector<expand_test_t> expand_absolute_test_list = {
    {"~/foo", env_util::get("HOME") + "/foo"},
    {"$HOME/foo", env_util::get("HOME") + "/foo"},
    {"/scratch/polybar", "/scratch/polybar"},
};

INSTANTIATE_TEST_SUITE_P(Inst, ExpandTest, ::testing::ValuesIn(expand_absolute_test_list));

TEST_P(ExpandTest, absolute) {
  EXPECT_EQ(file_util::expand(GetParam().first), GetParam().second);
}

TEST_P(ExpandTest, relativeToAbsolute) {
  EXPECT_EQ(file_util::expand(GetParam().first, "/scratch"), GetParam().second);
}

using expand_relative_test_t = std::tuple<string, string, string>;
class ExpandRelativeTest : public testing::TestWithParam<expand_relative_test_t> {};

vector<expand_relative_test_t> expand_relative_test_list = {
    {"../test", "/scratch", "/scratch/../test"},
    {"modules/battery", "/scratch/polybar", "/scratch/polybar/modules/battery"},
    {"/tmp/foo", "/scratch", "/tmp/foo"},
};

INSTANTIATE_TEST_SUITE_P(Inst, ExpandRelativeTest, ::testing::ValuesIn(expand_relative_test_list));

TEST_P(ExpandRelativeTest, correctness) {
  string path, relative_to, expected;
  std::tie(path, relative_to, expected) = GetParam();
  EXPECT_EQ(file_util::expand(path, relative_to), expected);
}
