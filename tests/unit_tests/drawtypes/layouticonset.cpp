#include "drawtypes/layouticonset.hpp"

#include "common/test.hpp"

using namespace std;
using namespace polybar;
using namespace polybar::drawtypes;

TEST(LayoutIconSet, get) {
  layouticonset_t layout_icons = make_shared<layouticonset>(make_shared<label>("default-icon"));

  EXPECT_EQ("default-icon", layout_icons->get("", "")->get());
  EXPECT_EQ("default-icon", layout_icons->get("any_layout", "")->get());
  EXPECT_EQ("default-icon", layout_icons->get("", "any_variant")->get());
  EXPECT_EQ("default-icon", layout_icons->get("any_layout", "any_variant")->get());

  // us;icon => layout 'us' with any variant
  EXPECT_TRUE(layout_icons->add("us", layouticonset::VARIANT_ANY, make_shared<label>("us--icon")));

  EXPECT_EQ("default-icon", layout_icons->get("", "")->get());
  EXPECT_EQ("default-icon", layout_icons->get("any_layout", "")->get());
  EXPECT_EQ("default-icon", layout_icons->get("", "any_variant")->get());
  EXPECT_EQ("default-icon", layout_icons->get("any_layout", "any_variant")->get());
  EXPECT_EQ("default-icon", layout_icons->get("Us", "")->get());

  EXPECT_EQ("us--icon", layout_icons->get("us", "")->get());
  EXPECT_EQ("us--icon", layout_icons->get("us", "undefined_variant")->get());

  // us;colemak;icon => layout 'us' with 'colemak' variant
  EXPECT_TRUE(layout_icons->add("us", "colemak", make_shared<label>("us-colemak-icon")));

  EXPECT_EQ("us--icon", layout_icons->get("us", "undefined_variant")->get());
  EXPECT_EQ("us-colemak-icon", layout_icons->get("us", "colemak")->get());
  EXPECT_EQ("us-colemak-icon", layout_icons->get("us", "COLEMAK")->get());
  EXPECT_EQ("us-colemak-icon", layout_icons->get("us", "a variant containing CoLeMaK in its description")->get());

  // us;;icon => layout 'us' with no variant
  EXPECT_TRUE(layout_icons->add("us", "", make_shared<label>("us-no_variant-icon")));

  EXPECT_EQ("us-no_variant-icon", layout_icons->get("us", "")->get());
  EXPECT_EQ("us--icon", layout_icons->get("us", "undefined_variant")->get());
  EXPECT_EQ("us-colemak-icon", layout_icons->get("us", "colemak")->get());
  EXPECT_EQ("us-colemak-icon", layout_icons->get("us", "COLEMAK")->get());
  EXPECT_EQ("us-colemak-icon", layout_icons->get("us", "a variant containing CoLeMaK in its description")->get());

  // _;dvorak;icon => any layout with 'dvorak' variant
  EXPECT_TRUE(layout_icons->add(layouticonset::VARIANT_ANY, "dvorak", make_shared<label>("any_layout-dvorak-icon")));
  EXPECT_EQ("any_layout-dvorak-icon", layout_icons->get("fr", "dvorak")->get());
  EXPECT_EQ("any_layout-dvorak-icon", layout_icons->get("fr", "dVORAk")->get());
  EXPECT_EQ("us--icon", layout_icons->get("us", "dvorak")->get());

  // us;dvorak;icon => layout 'us' with 'dvorak' variant
  EXPECT_TRUE(layout_icons->add("us", "dvorak", make_shared<label>("us-dvorak-icon")));
  EXPECT_EQ("any_layout-dvorak-icon", layout_icons->get("fr", "dvorak")->get());
  EXPECT_EQ("us-dvorak-icon", layout_icons->get("us", "dvorak")->get());

  // _;;icon => any layout with no variant
  EXPECT_TRUE(layout_icons->add(layouticonset::VARIANT_ANY, "", make_shared<label>("any_layout-no_variant-icon")));
  EXPECT_EQ("any_layout-no_variant-icon", layout_icons->get("fr", "")->get());
  EXPECT_EQ("us-no_variant-icon", layout_icons->get("us", "")->get());

  EXPECT_TRUE(layout_icons->add("us", "variant2", make_shared<label>("us-variant2-icon")));
  EXPECT_EQ("us-colemak-icon", layout_icons->get("us", "a variant containing CoLeMaK & variant2 in its description")->get());

  EXPECT_TRUE(layout_icons->add(layouticonset::VARIANT_ANY, "variant2", make_shared<label>("any_layout-variant2-icon")));
  EXPECT_EQ("any_layout-dvorak-icon", layout_icons->get("some layout", "a variant containing dvorak & variant2 in its description")->get());

  // us;_;icon => layout 'us' with any variant
  layouticonset_t layout_icons2 = make_shared<layouticonset>(make_shared<label>("default-icon"));
  EXPECT_TRUE(layout_icons2->add("us", "_", make_shared<label>("us-any_variant-icon")));
  EXPECT_EQ("us-any_variant-icon", layout_icons2->get("us", "")->get());
  EXPECT_EQ("us-any_variant-icon", layout_icons2->get("us", "whatever variant")->get());

  EXPECT_FALSE(layout_icons->add(
      layouticonset::VARIANT_ANY, layouticonset::VARIANT_ANY, make_shared<label>("any_layout-no_variant-icon")));
}
