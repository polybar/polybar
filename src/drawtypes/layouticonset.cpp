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

    /*
     * The minimal case that was matched.
     * Once a case is matched, this is updated and no case with the same or higher number can be matched again.
     */
    int min_case = 6;

    // Case 6: initializing with default
    label_t icon = m_default_icon;

    for (auto it : m_layout_icons) {
      const string& icon_layout = std::get<0>(it);
      const string& icon_variant = std::get<1>(it);
      label_t icon_label = std::get<2>(it);

      bool is_variant_match = icon_variant == variant;

      bool is_variant_any = icon_variant == VARIANT_ANY;

      bool is_variant_match_fuzzy =
          !is_variant_any && !icon_variant.empty() && string_util::contains_ignore_case(variant, icon_variant);

      // Which of the 6 match cases is matched here.
      int current_case = 6;

      if (icon_layout == layout) {
        if (is_variant_match) {
          // Case 1
          current_case = 1;
        } else if (is_variant_match_fuzzy) {
          // Case 2
          current_case = 2;
        } else if (is_variant_any) {
          // Case 3
          current_case = 3;
        }
      } else if (icon_layout == VARIANT_ANY) {
        if (is_variant_match) {
          // Case 4
          current_case = 4;
        } else if (is_variant_match_fuzzy) {
          // Case 5
          current_case = 5;
        }
      }

      /*
       * We matched with a higher priority than before -> update icon.
       */
      if (current_case < min_case) {
        icon = icon_label;
        min_case = current_case;
      }

      if (current_case == 1) {
        // Case 1: perfect match, we can break early
        break;
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
