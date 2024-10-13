#include "drawtypes/ramp.hpp"

#include "utils/factory.hpp"
#include "utils/math.hpp"

POLYBAR_NS

namespace drawtypes {

  void ramp::add(label_t&& icon) {
    m_icons.emplace_back(std::move(icon));  // Use std::move to optimize
  }

  void ramp::add(label_t&& icon, unsigned weight) {
    // Add the icon based on the specified weight
    for (unsigned i = 0; i < weight; ++i) {
      m_icons.emplace_back(icon);
    }
  }

  label_t ramp::get(size_t index) const {
    if (index >= m_icons.size()) {
      throw std::out_of_range("Index is out of range");
    }
    return m_icons[index];
  }

  label_t ramp::get_by_percentage(float percentage) const {
    if (percentage < 0.0f || percentage > 100.0f) {
      throw std::invalid_argument("Percentage must be between 0 and 100");
    }
    size_t index = static_cast<size_t>(percentage * m_icons.size() / 100.0f);
    return m_icons[math_util::cap<size_t>(index, 0, m_icons.size() - 1)];
  }

  label_t ramp::get_by_percentage_with_borders(float value, float min, float max) const {
    if (min >= max) {
      throw std::invalid_argument("Minimum must be less than maximum");
    }
    size_t index;

    if (value <= min) {
      index = 0;
    } else if (value >= max) {
      index = m_icons.size() - 1;
    } else {
      float percentage = math_util::percentage(value, min, max);
      index = static_cast<size_t>(percentage * (m_icons.size() - 2) / 100.0f) + 1;
      index = math_util::cap<size_t>(index, 0, m_icons.size() - 1);
    }
    return m_icons[index];
  }

  ramp::operator bool() const {
    return !m_icons.empty();
  }

  /**
   * Create a ramp by loading values from the configuration
   */
  ramp_t load_ramp(const config& conf, const string& section, string name, bool required) {
    name = string_util::ltrim(string_util::rtrim(std::move(name), '>'), '<');

    auto ramp_defaults = load_optional_label(conf, section, name);
    vector<label_t> vec;
    vector<string> icons = required ? conf.get_list<string>(section, name) : conf.get_list<string>(section, name, {});

    for (size_t i = 0; i < icons.size(); ++i) {
      auto ramp_name = name + "-" + std::to_string(i);
      auto icon = load_optional_label(conf, section, ramp_name, icons[i]);
      icon->copy_undefined(ramp_defaults);

      unsigned weight = conf.get(section, ramp_name + "-weight", 1U);
      for (unsigned j = 0; j < weight; ++j) {
        vec.emplace_back(icon);
      }
    }

    return std::make_shared<drawtypes::ramp>(std::move(vec));
  }
}  // namespace drawtypes

POLYBAR_NS_END
