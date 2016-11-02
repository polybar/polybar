#include "drawtypes/animation.hpp"
#include "drawtypes/label.hpp"

LEMONBUDDY_NS

namespace drawtypes {
  void animation::add(icon_t&& frame) {
    m_frames.emplace_back(forward<decltype(frame)>(frame));
    m_framecount = m_frames.size();
  }

  icon_t animation::get() {
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

    if (diff.count() < m_framerate_ms)
      return;
    if (++m_frame >= m_framecount)
      m_frame = 0;

    m_lastupdate = now;
  }

  /**
   * Create an animation by loading values
   * from the configuration
   */
  animation_t load_animation(const config& conf, string section, string name, bool required) {
    vector<icon_t> vec;
    vector<string> frames;

    name = string_util::ltrim(string_util::rtrim(name, '>'), '<');

    if (required)
      frames = conf.get_list<string>(section, name);
    else
      frames = conf.get_list<string>(section, name, {});

    for (size_t i = 0; i < frames.size(); i++)
      vec.emplace_back(
          forward<icon_t>(load_optional_icon(conf, section, name + "-" + to_string(i), frames[i])));

    auto framerate = conf.get<int>(section, name + "-framerate", 1000);

    return animation_t{new animation(move(vec), framerate)};
  }
}

LEMONBUDDY_NS_END
