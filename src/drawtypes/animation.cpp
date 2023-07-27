#include "drawtypes/animation.hpp"

#include "drawtypes/label.hpp"

POLYBAR_NS

namespace drawtypes {
  void animation::add(label_t&& frame) {
    m_frames.emplace_back(forward<decltype(frame)>(frame));
    m_framecount = m_frames.size();
    m_frame = m_framecount - 1;
  }

  label_t animation::get() const {
    return m_frames[m_frame];
  }

  unsigned int animation::framerate() const {
    return m_framerate_ms;
  }

  animation::operator bool() const {
    return !m_frames.empty();
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
  animation_t load_animation(const config::value& conf, bool required) {
    vector<label_t> vec;
    vector<string> frames;

    auto anim_defaults = load_optional_label(conf);

    if (required) {
      frames = conf.as_list<string>();
    } else {
      frames = conf.as_list<string>({});
    }

    for (size_t i = 0; i < frames.size(); i++) {
      vec.emplace_back(forward<label_t>(load_optional_label(conf[i], frames[i])));
      vec.back()->copy_undefined(anim_defaults);
    }

    auto framerate = conf["framerate"].as<unsigned int>(1000);

    return std::make_shared<animation>(move(vec), framerate);
  }
}  // namespace drawtypes

POLYBAR_NS_END
