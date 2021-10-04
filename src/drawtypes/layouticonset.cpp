#include "drawtypes/layouticonset.hpp"

POLYBAR_NS

namespace drawtypes {
  layouticonset::layouticonset(label_t&& default_icon) : m_default_icon(default_icon) {}

  bool layouticonset::add(const string& layout, const string& variant, label_t&& icon) {
    if (layout == VARIANT_ANY && variant == VARIANT_ANY) {
      return false;
    }
    m_layout_icons.emplace_back(layout, variant, icon);
    return true;
  }

  label_t layouticonset::get(const string& layout, const string& variant) const {
    // The layout, variant are matched against defined icons in that order:
    //     1. perfect match on layout and perfect match on variant (ex: us;Colemak;<icon>)
    //     2. perfect match on layout and case insensitive search on variant (ex: us;coLEmAk;<icon>)
    //     3. perfect match on layout and the any variant '_' (ex: us;<icon> or us;_;<icon>)
    //     4. any layout for icon and perfect match on variant (ex: _;Colemak;<icon>)
    //     5. any layout for icon and case insensitive search on variant (ex: _;coLEmAk;<icon>)
    //     6. no match at all => default icon if defined

    // Case 2:  Perfect layout + case insensitive variant match
    bool layout_ivariant_matched = false;
    // Case 3: Perfect layout + wildcard variant match
    bool layout_anyvariant_matched = false;
    // Case 4: Wildcard layout + perfect variant match
    bool anylayout_variant_matched = false;
    // Case 5: Wildcard layout + case insensitive variant match
    bool anylayout_ivariant_matched = false;

    // Case 6: initializing with default
    label_t icon = m_default_icon;
    for (auto it : m_layout_icons) {
      const string& icon_layout = std::get<0>(it);
      const string& icon_variant = std::get<1>(it);
      label_t icon_label = std::get<2>(it);

      bool variant_match_ignoring_case = icon_variant != VARIANT_ANY && !icon_variant.empty() &&
                                         string_util::contains_ignore_case(variant, icon_variant);

      if (icon_layout == layout) {
        if (icon_variant == variant) {
          // Case 1: perfect match, we can break
          icon = icon_label;
          break;
        } else if (variant_match_ignoring_case && !layout_ivariant_matched) {
          // Case 2
          layout_ivariant_matched = true;
          icon = icon_label;
        } else if (icon_variant == VARIANT_ANY && !layout_ivariant_matched) {
          // Case 3
          layout_anyvariant_matched = true;
          icon = icon_label;
        }
      } else if (icon_layout == VARIANT_ANY && !layout_ivariant_matched && !layout_anyvariant_matched) {
        if (icon_variant == variant) {
          // Case 4
          anylayout_variant_matched = true;
          icon = icon_label;
        } else if (variant_match_ignoring_case && !anylayout_variant_matched && !anylayout_ivariant_matched) {
          // Case 5
          anylayout_ivariant_matched = true;
          icon = icon_label;
        }
      }
    }
    return icon;
  }

  bool layouticonset::contains(const string& layout, const string& variant) const {
    for (auto it : m_layout_icons) {
      const string& icon_layout = std::get<0>(it);
      const string& icon_variant = std::get<1>(it);
      if (icon_layout == layout && icon_variant == variant) {
        return true;
      }
    }
    return false;
  }
}  // namespace drawtypes

POLYBAR_NS_END
