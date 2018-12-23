#include "drawtypes/ramp.hpp"
#include "utils/factory.hpp"
#include "utils/math.hpp"

POLYBAR_NS

namespace drawtypes {
  void ramp::add(label_t&& icon) {
    m_icons.emplace_back(forward<decltype(icon)>(icon));
  }

  label_t ramp::get(size_t index) {
    return m_icons[index];
  }

  label_t ramp::get_by_percentage(float percentage) {
    size_t index = percentage * m_icons.size() / 100.0f;
    return m_icons[math_util::cap<size_t>(index, 0, m_icons.size() - 1)];
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
      auto icon = load_optional_label(conf, section, name + "-" + to_string(i), icons[i]);
      icon->copy_undefined(ramp_defaults);
      vec.emplace_back(move(icon));
    }

    return factory_util::shared<drawtypes::ramp>(move(vec));
  }
}

POLYBAR_NS_END
