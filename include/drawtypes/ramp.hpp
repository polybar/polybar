#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "drawtypes/label.hpp"
#include "utils/math.hpp"
#include "utils/mixins.hpp"

LEMONBUDDY_NS

namespace drawtypes {
  class ramp : public non_copyable_mixin<ramp> {
   public:
    explicit ramp() = default;
    explicit ramp(vector<icon_t>&& icons) : m_icons(forward<decltype(icons)>(icons)) {}

    void add(icon_t&& icon) {
      m_icons.emplace_back(forward<decltype(icon)>(icon));
    }

    icon_t get(size_t index) {
      return m_icons[index];
    }

    icon_t get_by_percentage(float percentage) {
      size_t index = percentage * (m_icons.size() - 1) / 100.0f + 0.5f;
      return m_icons[math_util::cap<size_t>(index, 0, m_icons.size() - 1)];
    }

    operator bool() {
      return !m_icons.empty();
    }

   protected:
    vector<icon_t> m_icons;
  };

  using ramp_t = shared_ptr<ramp>;

  /**
   * Create a ramp by loading values
   * from the configuration
   */
  inline auto load_ramp(const config& conf, string section, string name, bool required = true) {
    name = string_util::ltrim(string_util::rtrim(name, '>'), '<');

    icon_t ramp_defaults;

    try {
      ramp_defaults = load_icon(conf, section, name);
    } catch (const key_error&) {
    }

    vector<icon_t> vec;
    vector<string> icons;

    if (required)
      icons = conf.get_list<string>(section, name);
    else
      icons = conf.get_list<string>(section, name, {});

    for (size_t i = 0; i < icons.size(); i++) {
      auto icon = load_optional_icon(conf, section, name + "-" + to_string(i), icons[i]);

      if (ramp_defaults)
        icon->copy_undefined(ramp_defaults);

      vec.emplace_back(move(icon));
    }

    return ramp_t{new ramp_t::element_type(move(vec))};
  }
}

LEMONBUDDY_NS_END
