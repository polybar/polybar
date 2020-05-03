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
