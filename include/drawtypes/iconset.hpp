#pragma once

#include "common.hpp"
#include "drawtypes/label.hpp"
#include "utils/mixins.hpp"

LEMONBUDDY_NS

namespace drawtypes {
  class iconset : public non_copyable_mixin<iconset> {
   public:
    void add(string id, icon_t&& icon) {
      m_icons.emplace(id, forward<decltype(icon)>(icon));
    }

    bool has(string id) {
      return m_icons.find(id) != m_icons.end();
    }

    icon_t get(string id, string fallback_id = "") {
      auto icon = m_icons.find(id);
      if (icon == m_icons.end())
        return m_icons.find(fallback_id)->second;
      return icon->second;
    }

    operator bool() {
      return m_icons.size() > 0;
    }

   protected:
    map<string, icon_t> m_icons;
  };

  using iconset_t = shared_ptr<iconset>;
}

LEMONBUDDY_NS_END
