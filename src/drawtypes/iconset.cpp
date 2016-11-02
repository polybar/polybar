#include "drawtypes/iconset.hpp"

LEMONBUDDY_NS

namespace drawtypes {
  void iconset::add(string id, icon_t&& icon) {
    m_icons.emplace(id, forward<decltype(icon)>(icon));
  }

  bool iconset::has(string id) {
    return m_icons.find(id) != m_icons.end();
  }

  icon_t iconset::get(string id, string fallback_id) {
    auto icon = m_icons.find(id);
    if (icon == m_icons.end())
      return m_icons.find(fallback_id)->second;
    return icon->second;
  }

  iconset::operator bool() {
    return m_icons.size() > 0;
  }
}

LEMONBUDDY_NS_END
