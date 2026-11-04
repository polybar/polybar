#include "drawtypes/iconset.hpp"

#include <algorithm>

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
      // works by finding the *longest* matching icon to the given workspace id
      size_t max_size = 0;
      label_t max_label;

      for (auto const& candidate : m_icons) {
        if (id.find(candidate.first) != std::string::npos) {
          if (!max_label || candidate.first.length() > max_size) {
            max_size = candidate.first.length();
            max_label = candidate.second;
          }
        }
      }

      if (max_label) {
        return max_label;
      }
    }

    // at() throws if the fallback is missing, instead of dereferencing end() (undefined behavior)
    return m_icons.at(fallback_id);
  }

  iconset::operator bool() {
    return !m_icons.empty();
  }
} // namespace drawtypes

POLYBAR_NS_END
