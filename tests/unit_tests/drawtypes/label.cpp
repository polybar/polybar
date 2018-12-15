#include <memory>

#include "common.hpp"
#include "common/test.hpp"
#include "components/types.hpp"
#include "drawtypes/label.hpp"

using namespace polybar;
using namespace polybar::drawtypes;

unique_ptr<label> create_alignment_test_label(string text, int minlen, alignment label_alignment) {
  // It's simpler in this case to create a label with all the constructor defaults and then set the relevant
  // fields individually.
  auto test_label = make_unique<label>(move(text));
  test_label->m_minlen = minlen;
  test_label->m_alignment = label_alignment;
  return test_label;
}

TEST(Label, no_alignment_needed) {
  EXPECT_EQ("abc", create_alignment_test_label("abc", 2, alignment::LEFT)->get());
  EXPECT_EQ("abc", create_alignment_test_label("abc", 2, alignment::CENTER)->get());
  EXPECT_EQ("abc", create_alignment_test_label("abc", 2, alignment::RIGHT)->get());
}

TEST(Label, left_alignment) {
  EXPECT_EQ("a ", create_alignment_test_label("a", 2, alignment::LEFT)->get());
  EXPECT_EQ("a  ", create_alignment_test_label("a", 3, alignment::LEFT)->get());
  EXPECT_EQ("abcde     ", create_alignment_test_label("abcde", 10, alignment::LEFT)->get());
}

TEST(Label, center_alignment) {
  EXPECT_EQ(" a ", create_alignment_test_label("a", 3, alignment::CENTER)->get());
  EXPECT_EQ("  a  ", create_alignment_test_label("a", 5, alignment::CENTER)->get());
  EXPECT_EQ("   abcd   ", create_alignment_test_label("abcd", 10, alignment::CENTER)->get());
}
