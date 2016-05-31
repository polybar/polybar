#pragma once

#include <string>
#include <vector>
#include <chrono>

#include "drawtypes/icon.hpp"

namespace drawtypes
{
  class Animation
  {
    std::vector<std::unique_ptr<Icon>> frames;
    int num_frames = 0;
    int current_frame= 0;
    int framerate_ms = 1000;
    std::chrono::system_clock::time_point updated_at;

    void tick();

    public:
      Animation(std::vector<std::unique_ptr<Icon>> &&frames, int framerate_ms = 1);
      explicit Animation(int framerate_ms)
        : framerate_ms(framerate_ms){}

      void add(std::unique_ptr<Icon> &&frame);

      std::unique_ptr<Icon> &get();

      int get_framerate();

      operator bool() {
        return !this->frames.empty();
      }
  };

  std::unique_ptr<Animation> get_config_animation(const std::string& config_path, const std::string& animation_name = "animation", bool required = true);
}
