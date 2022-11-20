#include "drawtypes/iconset.hpp"

#include "common/test.hpp"

using namespace std;
using namespace polybar;
using namespace polybar::drawtypes;

TEST(IconSet, fuzzyMatchExactMatchFirst) {
  iconset_t icons = make_shared<iconset>();

  icons->add("1", make_shared<label>("1"));
  icons->add("10", make_shared<label>("10"));

  label_t ret = icons->get("10", "", true);

  EXPECT_EQ("10", ret->get());
}

TEST(IconSet, fuzzyMatchLargestSubstring) {
  iconset_t icons = make_shared<iconset>();

  icons->add("1", make_shared<label>("1"));
  icons->add("10", make_shared<label>("10"));

  label_t ret = icons->get("10a", "", true);

  EXPECT_EQ("10", ret->get());
}

TEST(IconSet, fuzzyMatchFallback) {
  iconset_t icons = make_shared<iconset>();

  icons->add("1", make_shared<label>("1"));
  icons->add("10", make_shared<label>("10"));
  icons->add("fallback_id", make_shared<label>("fallback_label"));

  label_t ret = icons->get("b", "fallback_id", true);

  EXPECT_EQ("fallback_label", ret->get());
}
