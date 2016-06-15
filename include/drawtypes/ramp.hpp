#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "drawtypes/label.hpp"
#include "utils/mixins.hpp"

LEMONBUDDY_NS

namespace drawtypes {
  class ramp;
  using ramp_t = shared_ptr<ramp>;

  class ramp : public non_copyable_mixin<ramp> {
   public:
    explicit ramp() = default;
    explicit ramp(vector<icon_t>&& icons) : m_icons(forward<decltype(icons)>(icons)) {}

    void add(icon_t&& icon) {
      m_icons.emplace_back(forward<decltype(icon)>(icon));
    }

    icon_t get(int index) {
      return m_icons[index];
    }

    icon_t get_by_percentage(float percentage) {
      return m_icons[static_cast<int>(percentage * (m_icons.size() - 1) / 100.0f + 0.5f)];
    }

    operator bool() {
      return m_icons.size() > 0;
    }

   protected:
    vector<icon_t> m_icons;
  };

  inline auto get_config_ramp(
      const config& conf, string section, string name = "ramp", bool required = true) {
    vector<icon_t> vec;

    name = string_util::ltrim(string_util::rtrim(name, '>'), '<');

    vector<string> icons;

    if (required)
      icons = conf.get_list<string>(section, name);
    else
      icons = conf.get_list<string>(section, name, {});

    auto foreground = conf.get<string>(section, name + "-foreground", "");
    for (int i = 0; i < (int)icons.size(); i++) {
      auto ramp = name + "-" + to_string(i);
      auto icon = get_optional_config_icon(conf, section, ramp, icons[i]);
      if (icon->m_foreground.empty() && !foreground.empty())
        icon->m_foreground = foreground;
      vec.emplace_back(std::move(icon));
    }

    return ramp_t{new ramp(std::move(vec))};
  }
}

LEMONBUDDY_NS_END
