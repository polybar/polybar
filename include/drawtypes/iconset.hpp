#pragma once

#include <map>

#include "common.hpp"
#include "drawtypes/label.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace drawtypes {
  class iconset : public non_copyable_mixin<iconset> {
   public:
    void add(string id, icon_t&& icon);
    bool has(const string& id);
    icon_t get(const string& id, const string& fallback_id = "", bool fuzzy_match = false);
    operator bool();

   protected:
    std::map<string, icon_t> m_icons;
  };

  using iconset_t = shared_ptr<iconset>;
}

POLYBAR_NS_END
