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
    explicit ramp(vector<label_t>&& icons) : m_icons(forward<decltype(icons)>(icons)) {}

    void add(label_t&& icon);
    label_t get(size_t index);
    label_t get_by_percentage(float percentage);
    operator bool();

   protected:
    vector<label_t> m_icons;
  };

  using ramp_t = shared_ptr<ramp>;

  ramp_t load_ramp(const config& conf, const string& section, string name, bool required = true);
}

POLYBAR_NS_END
