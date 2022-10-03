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
    fuzzy_match = true; 

    for(auto const& icon : m_icons){
      std::cout << icon.first << " ";
    }

    std::cout << std::endl << id << std::endl;
    //std::cout << "HI: " << fallback_id << std::endl;

    // Try to match exactly first
    auto icon = m_icons.find(id);
    if (icon != m_icons.end()) {
      return icon->second;
    }

    // If fuzzy matching is turned on, try that first before returning the fallback.
    if (fuzzy_match) {
      // works by finding the *longest* matching icon to the given workspace id
      int max_size = -1;
      for (auto const& icon : m_icons) {
        if (id.find(icon.first) != std::string::npos) {
          max_size = std::max(max_size, (int)icon.first.length());
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
