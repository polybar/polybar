#include "drawtypes/label.hpp"

#include <memory>
#include <tuple>

#include "common.hpp"
#include "common/test.hpp"
#include "components/types.hpp"

using namespace polybar;
using namespace std;
using namespace polybar::drawtypes;

using GetTestParamLabel = tuple<string, bool, int, int, alignment>;
using GetTestParam = pair<string, GetTestParamLabel>;
/**
 * \brief Class for parameterized tests label::get
 *
 * The first element of the pair is the expected returned text, the second
 * element is a 5-tuple (text, ellipsis, minlen, maxlen, alignment)
 */
class GetTest : public ::testing::Test, public ::testing::WithParamInterface<GetTestParam> {};

vector<GetTestParam> get_list = {
    {"...", make_tuple("abcd", true, 0, 3, alignment::RIGHT)},
    {"abc", make_tuple("abc", true, 0, 3, alignment::RIGHT)},
    {"abc", make_tuple("abcdefgh", false, 0, 3, alignment::RIGHT)},
    {"a...", make_tuple("abcdefgh", true, 0, 4, alignment::RIGHT)},
    {"abcd...", make_tuple("abcdefgh", true, 0, 7, alignment::RIGHT)},
    {"abcdefgh", make_tuple("abcdefgh", true, 0, 8, alignment::RIGHT)},
};
INSTANTIATE_TEST_SUITE_P(Inst, GetTest, ::testing::ValuesIn(get_list));

// No alignment needed
vector<GetTestParam> get_no_align_list = {
    {"abc", make_tuple("abc", true, 3, 0, alignment::LEFT)},
    {"abc", make_tuple("abc", true, 2, 0, alignment::CENTER)},
    {"abc", make_tuple("abc", true, 1, 0, alignment::RIGHT)},
};

INSTANTIATE_TEST_SUITE_P(NoAlignment, GetTest, ::testing::ValuesIn(get_no_align_list));

// Left alignment
vector<GetTestParam> get_left_align_list = {
    {"a ", make_tuple("a", true, 2, 0, alignment::LEFT)},
    {"a  ", make_tuple("a", true, 3, 0, alignment::LEFT)},
    {"abcde     ", make_tuple("abcde", true, 10, 0, alignment::LEFT)},
};

INSTANTIATE_TEST_SUITE_P(LeftAlignment, GetTest, ::testing::ValuesIn(get_left_align_list));

// Center alignment
vector<GetTestParam> get_center_align_list = {
    {" a ", make_tuple("a", true, 3, 0, alignment::CENTER)},
    {"  a  ", make_tuple("a", true, 5, 0, alignment::CENTER)},
    {"   abcd   ", make_tuple("abcd", true, 10, 0, alignment::CENTER)},
};

INSTANTIATE_TEST_SUITE_P(CenterAlignment, GetTest, ::testing::ValuesIn(get_center_align_list));

// Right alignment
vector<GetTestParam> get_right_align_list = {
    {"  a", make_tuple("a", true, 3, 0, alignment::RIGHT)},
    {" abc", make_tuple("abc", true, 4, 0, alignment::RIGHT)},
    {"       abc", make_tuple("abc", true, 10, 0, alignment::RIGHT)},
};

INSTANTIATE_TEST_SUITE_P(RightAlignment, GetTest, ::testing::ValuesIn(get_right_align_list));

vector<GetTestParam> get_min_max_list = {
    {"a ", make_tuple("a", true, 2, 2, alignment::CENTER)},
    {"abc ", make_tuple("abc", true, 4, 4, alignment::CENTER)},
    {"abc", make_tuple("abcd", false, 3, 3, alignment::RIGHT)},
    {"...", make_tuple("abcd", true, 1, 3, alignment::RIGHT)},
    {" ", make_tuple("", true, 1, 3, alignment::RIGHT)},
    {" a", make_tuple("a", true, 2, 3, alignment::RIGHT)},
    {"...", make_tuple("....", true, 2, 3, alignment::RIGHT)},
    {"...", make_tuple("....", false, 2, 3, alignment::RIGHT)},
    {"abc...", make_tuple("abcdefg", true, 6, 6, alignment::RIGHT)},
};

INSTANTIATE_TEST_SUITE_P(MinMax, GetTest, ::testing::ValuesIn(get_min_max_list));

unique_ptr<label> create_alignment_test_label(GetTestParamLabel params) {
  /* It's simpler in this case to create a label with all the constructor defaults and then set the relevant
   * fields individually.
   */
  auto test_label = make_unique<label>(get<0>(params));
  test_label->m_ellipsis = get<1>(params);
  test_label->m_minlen = get<2>(params);
  test_label->m_maxlen = get<3>(params);
  test_label->m_alignment = get<4>(params);
  return test_label;
}

TEST_P(GetTest, correctness) {
  auto m_label = create_alignment_test_label(GetParam().second);

  auto expected = GetParam().first;
  auto actual = m_label->get();
  EXPECT_EQ(expected, actual);
}

TEST_P(GetTest, soundness) {
  auto m_label = create_alignment_test_label(GetParam().second);
  auto actual = m_label->get();
  EXPECT_TRUE(m_label->m_maxlen == 0 || actual.length() <= m_label->m_maxlen) << "Returned text is longer than maxlen";
  EXPECT_GE(actual.length(), m_label->m_minlen) << "Returned text is shorter than minlen";
}
