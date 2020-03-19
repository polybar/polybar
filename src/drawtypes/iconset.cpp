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
    // Try to match exactly first
    auto icon = m_icons.find(id);
    if (icon != m_icons.end()) {
      return icon->second;
    }

    // If fuzzy matching is turned on, try that first before returning the fallback.
    if (fuzzy_match) {
      for (auto const& icon : m_icons) {
        if (id.find(icon.first) != std::string::npos) {
          return icon.second;
        }
      }
    }

    return m_icons.find(fallback_id)->second;
  }

  iconset::operator bool() {
    return !m_icons.empty();
  }
}  // namespace drawtypes

POLYBAR_NS_END
