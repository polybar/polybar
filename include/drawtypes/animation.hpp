#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "drawtypes/label.hpp"
#include "utils/mixins.hpp"

LEMONBUDDY_NS

namespace drawtypes {
  class animation;
  using animation_t = shared_ptr<animation>;

  class animation : public non_copyable_mixin<animation> {
   public:
    explicit animation(int framerate_ms) : m_framerate_ms(framerate_ms) {}
    explicit animation(vector<icon_t>&& frames, int framerate_ms)
        : m_frames(forward<decltype(frames)>(frames))
        , m_framerate_ms(framerate_ms)
        , m_framecount(m_frames.size())
        , m_lastupdate(chrono::system_clock::now()) {}

    void add(icon_t&& frame) {
      m_frames.emplace_back(forward<decltype(frame)>(frame));
      m_framecount = m_frames.size();
    }

    icon_t get() {
      tick();
      return m_frames[m_frame];
    }

    int framerate() {
      return m_framerate_ms;
    }

    operator bool() {
      return !m_frames.empty();
    }

   protected:
    vector<icon_t> m_frames;
    int m_framerate_ms = 1000;
    int m_frame = 0;
    int m_framecount = 0;
    chrono::system_clock::time_point m_lastupdate;

    void tick() {
      auto now = chrono::system_clock::now();
      auto diff = chrono::duration_cast<chrono::milliseconds>(now - m_lastupdate);

      if (diff.count() < m_framerate_ms)
        return;
      if (++m_frame >= m_framecount)
        m_frame = 0;

      m_lastupdate = now;
    }
  };

  inline auto get_config_animation(
      const config& conf, string section, string name = "animation", bool required = true) {
    vector<icon_t> vec;
    vector<string> frames;

    name = string_util::ltrim(string_util::rtrim(name, '>'), '<');

    if (required)
      frames = conf.get_list<string>(section, name);
    else
      frames = conf.get_list<string>(section, name, {});

    for (int i = 0; i < (int)frames.size(); i++)
      vec.emplace_back(forward<icon_t>(
          get_optional_config_icon(conf, section, name + "-" + to_string(i), frames[i])));

    auto framerate = conf.get<int>(section, name + "-framerate", 1000);

    return animation_t{new animation(move(vec), framerate)};
  }
}

LEMONBUDDY_NS_END
