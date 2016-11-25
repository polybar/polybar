#include "drawtypes/iconset.hpp"

POLYBAR_NS

namespace drawtypes {
  void iconset::add(string id, icon_t&& icon) {
    m_icons.emplace(id, forward<decltype(icon)>(icon));
  }

  bool iconset::has(const string& id) {
    return m_icons.find(id) != m_icons.end();
  }

  icon_t iconset::get(const string& id, const string& fallback_id) {
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
