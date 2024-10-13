#include "drawtypes/ramp.hpp"
#include "utils/factory.hpp"
#include "utils/math.hpp"

POLYBAR_NS

namespace drawtypes {

  void ramp::add(label_t&& icon) {
    m_icons.emplace_back(std::forward<label_t>(icon));
  }

  void ramp::add(label_t&& icon, unsigned weight) {
    for (unsigned i = 0; i < weight; ++i) {
      m_icons.emplace_back(icon);
    }
  }

  label_t ramp::get(size_t index) const {
    // Ensure index is within bounds
    if (index >= m_icons.size()) {
      throw std::out_of_range("Index out of range");
    }
    return m_icons[index];
  }

  label_t ramp::get_by_percentage(float percentage) const {
    // Clamp percentage to the range [0, 100]
    percentage = std::clamp(percentage, 0.0f, 100.0f);
    size_t index = static_cast<size_t>(percentage * m_icons.size() / 100.0f);
    return m_icons[index];
  }

  label_t ramp::get_by_percentage_with_borders(float value, float min, float max) const {
    if (value <= min) {
      return m_icons.front();  // Return first icon if below minimum
    } else if (value >= max) {
      return m_icons.back();   // Return last icon if above maximum
    } else {
      float percentage = math_util::percentage(value, min, max);
      size_t index = static_cast<size_t>((percentage * (m_icons.size() - 2) / 100.0f) + 1);
      return m_icons[math_util::cap<size_t>(index, 0, m_icons.size() - 1)];
    }
  }

  ramp::operator bool() const {
    return !m_icons.empty();
  }

  /**
   * Create a ramp by loading values from the configuration
   */
  ramp_t load_ramp(const config& conf, const string& section, string name, bool required) {
    name = string_util::trim(name, "<>");

    auto ramp_defaults = load_optional_label(conf, section, name);

    vector<label_t> vec;
    vector<string> icons = required 
      ? conf.get_list<string>(section, name) 
      : conf.get_list<string>(section, name, {});

    for (size_t i = 0; i < icons.size(); ++i) {
      auto ramp_name = name + "-" + std::to_string(i);
      auto icon = load_optional_label(conf, section, ramp_name, icons[i]);
      icon->copy_undefined(ramp_defaults);

      auto weight = conf.get(section, ramp_name + "-weight", 1U);
      for (unsigned j = 0; j < weight; ++j) {
        vec.emplace_back(icon);
      }
    }

    return std::make_shared<drawtypes::ramp>(std::move(vec));
  }
}  // namespace drawtypes

POLYBAR_NS_END
