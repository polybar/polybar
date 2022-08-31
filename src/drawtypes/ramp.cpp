#include "drawtypes/ramp.hpp"

#include "utils/factory.hpp"
#include "utils/math.hpp"

POLYBAR_NS

namespace drawtypes {
  void ramp::add(label_t&& icon) {
    m_icons.emplace_back(forward<decltype(icon)>(icon));
  }

  void ramp::add(label_t&& icon, unsigned weight) {
    while (weight--) {
      m_icons.emplace_back(icon);
    }
  }

  label_t ramp::get(size_t index) {
    return m_icons[index];
  }

  label_t ramp::get_by_percentage(float percentage) {
    size_t index = percentage * m_icons.size() / 100.0f;
    return m_icons[math_util::cap<size_t>(index, 0, m_icons.size() - 1)];
  }

  label_t ramp::get_by_percentage_with_borders(int value, int min, int max) {
    return get_by_percentage_with_borders(static_cast<float>(value), static_cast<float>(min), static_cast<float>(max));
  }

  label_t ramp::get_by_percentage_with_borders(float value, float min, float max) {
    size_t index;
    if (value <= min) {
      index = 0;
    } else if (value >= max) {
      index = m_icons.size() - 1;
    } else {
      float percentage = math_util::percentage(value, min, max);
      index = percentage * (m_icons.size() - 2) / 100.0f + 1;
      index = math_util::cap<size_t>(index, 0, m_icons.size() - 1);
    }
    return m_icons[index];
  }

  ramp::operator bool() {
    return !m_icons.empty();
  }

  /**
   * Create a ramp by loading values
   * from the configuration
   */
  ramp_t load_ramp(const config& conf, const string& section, string name, bool required) {
    name = string_util::ltrim(string_util::rtrim(move(name), '>'), '<');

    auto ramp_defaults = load_optional_label(conf, section, name);

    vector<label_t> vec;
    vector<string> icons;

    if (required) {
      icons = conf.get_list<string>(section, name);
    } else {
      icons = conf.get_list<string>(section, name, {});
    }

    for (size_t i = 0; i < icons.size(); i++) {
      auto ramp_name = name + "-" + to_string(i);
      auto icon = load_optional_label(conf, section, ramp_name, icons[i]);
      icon->copy_undefined(ramp_defaults);

      auto weight = conf.get(section, ramp_name + "-weight", 1U);
      while (weight--) {
        vec.emplace_back(icon);
      }
    }

    return std::make_shared<drawtypes::ramp>(move(vec));
  }
}  // namespace drawtypes

POLYBAR_NS_END
