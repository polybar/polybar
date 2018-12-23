#pragma once

#include <chrono>

#include "common.hpp"
#include "components/config.hpp"
#include "drawtypes/label.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace chrono = std::chrono;

namespace drawtypes {
  class animation : public non_copyable_mixin<animation> {
   public:
    explicit animation(int framerate_ms) : m_framerate_ms(framerate_ms) {}
    explicit animation(vector<label_t>&& frames, int framerate_ms)
        : m_frames(forward<decltype(frames)>(frames))
        , m_framerate_ms(framerate_ms)
        , m_framecount(m_frames.size())
        , m_lastupdate(chrono::system_clock::now()) {}

    void add(label_t&& frame);
    label_t get();
    int framerate();
    operator bool();

   protected:
    void tick();

    vector<label_t> m_frames;
    int m_framerate_ms = 1000;
    int m_frame = 0;
    int m_framecount = 0;
    chrono::system_clock::time_point m_lastupdate;
  };

  using animation_t = shared_ptr<animation>;

  animation_t load_animation(
      const config& conf, const string& section, string name = "animation", bool required = true);
}

POLYBAR_NS_END
