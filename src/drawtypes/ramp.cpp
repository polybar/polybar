#include "drawtypes/ramp.hpp"
#include "utils/factory.hpp"
#include "utils/math.hpp"

POLYBAR_NS

namespace drawtypes {
  void ramp::add(label_t&& icon) {
    m_labels.emplace_back(forward<decltype(icon)>(icon));
  }

  label_t ramp::get(size_t index) {
    return m_labels[index];
  }

  label_t ramp::get_by_percentage(float percentage) {
    size_t index = percentage * m_labels.size() / 100.0f;
    return get(math_util::cap<size_t>(index, 0, m_labels.size() - 1));
  }

  label_t ramp::get_by_percentage_with_borders(int value, int min, int max) {
    return get_by_percentage_with_borders(static_cast<float>(value), static_cast<float>(min), static_cast<float>(max));
  }

  label_t ramp::get_by_percentage_with_borders(float value, float min, float max) {
    size_t index;
    if (value <= min) {
      index = 0;
    } else if (value >= max) {
      index = m_labels.size() - 1;
    } else {
      float percentage = math_util::percentage(value, min, max);
      index = percentage * (m_labels.size() - 2) / 100.0f + 1;
      index = math_util::cap<size_t>(index, 0, m_labels.size() - 1);
    }
    return m_labels[index];
  }

  ramp::operator bool() {
    return !m_labels.empty();
  }

  /**
   * Create a ramp by loading values
   * from the configuration
   */
  ramp_t load_ramp(const config& conf, const string& section, string name, bool required) {
    name = string_util::ltrim(string_util::rtrim(move(name), '>'), '<');
    vector<label_t> vec;

    load_labellist(vec, conf, section, name, required);

    return factory_util::shared<drawtypes::ramp>(move(vec));
  }
}

POLYBAR_NS_END
