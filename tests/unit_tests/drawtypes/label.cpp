#include <memory>

#include "common.hpp"
#include "common/test.hpp"
#include "components/types.hpp"
#include "drawtypes/label.hpp"

using namespace polybar;
using namespace polybar::drawtypes;

unique_ptr<label> create_alignment_test_label(string text, int minlen, int maxlen, alignment label_alignment) {
  // It's simpler in this case to create a label with all the constructor defaults and then set the relevant
  // fields individually.
  auto test_label = make_unique<label>(move(text));
  test_label->m_minlen = minlen;
  test_label->m_maxlen = maxlen;
  test_label->m_alignment = label_alignment;
  return test_label;
}

TEST(Label, no_alignment_needed) {
  EXPECT_EQ("abc", create_alignment_test_label("abc", 2, 0_z, alignment::LEFT)->get());
  EXPECT_EQ("abc", create_alignment_test_label("abc", 2, 0_z, alignment::CENTER)->get());
  EXPECT_EQ("abc", create_alignment_test_label("abc", 2, 0_z, alignment::RIGHT)->get());
}

TEST(Label, left_alignment) {
  EXPECT_EQ("a ", create_alignment_test_label("a", 2, 0_z, alignment::LEFT)->get());
  EXPECT_EQ("a  ", create_alignment_test_label("a", 3, 0_z, alignment::LEFT)->get());
  EXPECT_EQ("abcde     ", create_alignment_test_label("abcde", 10, 0_z, alignment::LEFT)->get());
}

TEST(Label, center_alignment) {
  EXPECT_EQ(" a ", create_alignment_test_label("a", 3, 0_z, alignment::CENTER)->get());
  EXPECT_EQ("  a  ", create_alignment_test_label("a", 5, 0_z, alignment::CENTER)->get());
  EXPECT_EQ("   abcd   ", create_alignment_test_label("abcd", 10, 0_z, alignment::CENTER)->get());
}

TEST(Label, min_max_center_alignment) {
  EXPECT_EQ("a ", create_alignment_test_label("a", 2, 2, alignment::CENTER)->get());
  EXPECT_EQ("abc ", create_alignment_test_label("abc", 4, 4, alignment::CENTER)->get());
}
