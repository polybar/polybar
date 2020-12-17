#include "drawtypes/animation.hpp"
#include "drawtypes/label.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

namespace drawtypes {
  void animation::add(label_t&& frame) {
    m_labels.emplace_back(forward<decltype(frame)>(frame));
    m_framecount = m_labels.size();
    m_frame = m_framecount - 1;
  }

  label_t animation::get() const {
    return m_labels[m_frame];
  }

  unsigned int animation::framerate() const {
    return m_framerate_ms;
  }

  animation::operator bool() const {
    return !m_labels.empty();
  }

  void animation::increment() {
    auto tmp = m_frame.load();
    ++tmp;
    tmp %= m_framecount;

    m_frame = tmp;
  }

  /**
   * Create an animation by loading values
   * from the configuration
   */
  animation_t load_animation(const config& conf, const string& section, string name, bool required) {
    name = string_util::ltrim(string_util::rtrim(move(name), '>'), '<');
    vector<label_t> vec;
    label_t tmplate;

    load_labellist(vec, tmplate, conf, section, name, required);

    auto framerate = conf.get(section, name + "-framerate", 1000);

    return factory_util::shared<animation>(move(vec), framerate, move(tmplate));
  }
}  // namespace drawtypes

POLYBAR_NS_END
