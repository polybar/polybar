#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "drawtypes/label.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace drawtypes {
  class ramp : public non_copyable_mixin<ramp> {
   public:
    explicit ramp() = default;
    explicit ramp(vector<icon_t>&& icons) : m_icons(forward<decltype(icons)>(icons)) {}

    void add(icon_t&& icon);
    icon_t get(size_t index);
    icon_t get_by_percentage(float percentage);
    operator bool();

   protected:
    vector<icon_t> m_icons;
  };

  using ramp_t = shared_ptr<ramp>;

  ramp_t load_ramp(const config& conf, const string& section, string name, bool required = true);
}

POLYBAR_NS_END
