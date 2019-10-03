#include "drawtypes/animation.hpp"
#include "drawtypes/label.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

namespace drawtypes {
  void animation::add(label_t&& frame) {
    m_frames.emplace_back(forward<decltype(frame)>(frame));
    m_framecount = m_frames.size();
  }

  label_t animation::get() {
    tick();
    return m_frames[m_frame];
  }

  int animation::framerate() {
    return m_framerate_ms;
  }

  animation::operator bool() {
    return !m_frames.empty();
  }

  void animation::tick() {
    auto now = chrono::system_clock::now();
    auto diff = chrono::duration_cast<chrono::milliseconds>(now - m_lastupdate);

    if (diff.count() < m_framerate_ms) {
      return;
    }
    if (++m_frame >= m_framecount) {
      m_frame = 0;
    }

    m_lastupdate = now;
  }

  /**
   * Create an animation by loading values
   * from the configuration
   */
  animation_t load_animation(const config& conf, const string& section, string name, bool required) {
    vector<label_t> vec;
    vector<string> frames;

    name = string_util::ltrim(string_util::rtrim(move(name), '>'), '<');

    auto anim_defaults = load_optional_label(conf, section, name);

    if (required) {
      frames = conf.get_list(section, name);
    } else {
      frames = conf.get_list(section, name, {});
    }

    for (size_t i = 0; i < frames.size(); i++) {
      vec.emplace_back(forward<label_t>(load_optional_label(conf, section, name + "-" + to_string(i), frames[i])));
      vec.back()->copy_undefined(anim_defaults);
    }

    auto framerate = conf.get(section, name + "-framerate", 1000);

    return factory_util::shared<animation>(move(vec), framerate);
  }
}

POLYBAR_NS_END
