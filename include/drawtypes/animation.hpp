#pragma once

#include <atomic>
#include <chrono>

#include "common.hpp"
#include "components/config.hpp"
#include "drawtypes/label.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace chrono = std::chrono;

namespace drawtypes {
  class animation : public non_copyable_mixin {
   public:
    explicit animation(unsigned int framerate_ms) : m_framerate_ms(framerate_ms) {}
    explicit animation(vector<label_t>&& frames, int framerate_ms)
        : m_frames(move(frames))
        , m_framerate_ms(framerate_ms)
        , m_framecount(m_frames.size())
        , m_frame(m_frames.size() - 1) {}

    void add(label_t&& frame);
    void increment();

    label_t get() const;
    unsigned int framerate() const;

    explicit operator bool() const;

   protected:
    vector<label_t> m_frames;

    unsigned int m_framerate_ms = 1000;
    size_t m_framecount = 0;
    std::atomic_size_t m_frame{0_z};
  };

  using animation_t = shared_ptr<animation>;

  animation_t load_animation(
      const config& conf, const string& section, string name = "animation", bool required = true);
}  // namespace drawtypes

POLYBAR_NS_END
