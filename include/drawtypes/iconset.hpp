#pragma once

#include "common.hpp"
#include "drawtypes/label.hpp"
#include "utils/mixins.hpp"

LEMONBUDDY_NS

namespace drawtypes {
  class iconset : public non_copyable_mixin<iconset> {
   public:
    void add(string id, icon_t&& icon);
    bool has(string id);
    icon_t get(string id, string fallback_id = "");
    operator bool();

   protected:
    map<string, icon_t> m_icons;
  };

  using iconset_t = shared_ptr<iconset>;
}

LEMONBUDDY_NS_END
