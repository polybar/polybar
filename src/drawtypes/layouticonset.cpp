#include "drawtypes/layouticonset.hpp"

POLYBAR_NS

namespace drawtypes {
  layouticonset::layouticonset(label_t&& default_icon) : m_default_icon(default_icon) {
  }

  void layouticonset::add(const string& layout, const string& variant, label_t&& icon) {
    m_layout_icons.push_back(std::make_tuple(layout, variant, icon));
  }

  label_t layouticonset::get(const string& layout, const string& variant) const {
    // The layout, variant are matched against defined icons in that order:
    //     1. perfect match on layout and perfect match on variant (ex: us;Colemak;<icon>)
    //     2. perfect match on layout and case insensitive search on variant (ex: us;coLEmAk;<icon>)
    //     3. perfect match on layout and the any variant '_' (ex: us;<icon> or us;_;<icon>)
    //     4. no layout for icon and perfect match on variant (ex: _;Colemak;<icon>)
    //     5. no layout for icon and case insensitive search on variant (ex: _;coLEmAk;<icon>)
    //     6. no match at all => default icon if defined
    bool layout_ivariant_matched = false;
    bool layout_wildcardvariant_matched = false;
    bool layout_novariant_matched = false;
    bool nolayout_variant_matched = false;

    auto icon = m_default_icon;
    for (auto it : m_layout_icons) {
      const string& icon_layout = std::get<0>(it);
      const string& icon_variant = std::get<1>(it);
      label_t icon_label = std::get<2>(it);

      if (icon_layout == layout) {
        if (icon_variant == variant || (icon_variant == VARIANT_NONE && variant.empty())) {
          // perfect match, we can break
          icon = icon_label;
          break;
        } else if (icon_variant != VARIANT_ANY && string_util::contains_nocase(variant, icon_variant)) {
          layout_ivariant_matched = true;
          icon = icon_label;
        } else if (!layout_ivariant_matched) {
          if (icon_variant == VARIANT_ANY) {
            layout_wildcardvariant_matched = true;
            icon = icon_label;
          } else if (!layout_wildcardvariant_matched && icon_variant == VARIANT_NONE && variant.empty()) {
            layout_novariant_matched = true;
            icon = icon_label;
          }
        }
      } else if (!(layout_ivariant_matched || layout_wildcardvariant_matched || layout_novariant_matched) &&
                 icon_layout == VARIANT_ANY) {
        if (icon_variant == variant || (icon_variant == VARIANT_NONE && variant.empty())) {
          nolayout_variant_matched = true;
          icon = icon_label;
        } else if (!nolayout_variant_matched && icon_variant != VARIANT_ANY &&
                   string_util::contains_nocase(variant, icon_variant)) {
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
