#include "drawtypes/iconset.hpp"

POLYBAR_NS

namespace drawtypes {
  void iconset::add(string id, label_t&& icon) {
    m_icons.emplace(id, forward<decltype(icon)>(icon));
  }

  bool iconset::has(const string& id) {
    return m_icons.find(id) != m_icons.end();
  }

  label_t iconset::get(const string& id, const string& fallback_id, bool fuzzy_match) {
    if (fuzzy_match) {
      for (auto const& ent1 : m_icons) {
        if (id.find(ent1.first) != std::string::npos) {
          return ent1.second;
        }
      }
      return m_icons.find(fallback_id)->second;
    }

    // Not fuzzy matching so use old method which requires an exact match on icon id
    auto icon = m_icons.find(id);
    if (icon == m_icons.end()) {
      return m_icons.find(fallback_id)->second;
    }
    return icon->second;
  }

  iconset::operator bool() {
    return !m_icons.empty();
  }
}

POLYBAR_NS_END
