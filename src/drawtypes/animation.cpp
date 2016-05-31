#include <string>
#include <vector>

#include "drawtypes/animation.hpp"
#include "utils/config.hpp"
#include "utils/memory.hpp"

namespace drawtypes
{
  std::unique_ptr<Animation> get_config_animation(const std::string& config_path, const std::string& animation_name, bool required)
  {
    std::vector<std::unique_ptr<Icon>> vec;
    std::vector<std::string> frames;

    if (required)
      frames = config::get_list<std::string>(config_path, animation_name);
    else
      frames = config::get_list<std::string>(config_path, animation_name, {});

    auto n_frames = frames.size();

    repeat(n_frames)
    {
      auto anim = animation_name +":"+ std::to_string(repeat_i_rev(n_frames));
      vec.emplace_back(std::unique_ptr<Icon> { get_optional_config_icon(config_path, anim, frames[n_frames - repeat_i - 1]) });
    }

    auto framerate = config::get<int>(config_path, animation_name +":framerate_ms", 1000);

    return std::unique_ptr<Animation> { new Animation(std::move(vec), framerate) };
  }

  Animation::Animation(std::vector<std::unique_ptr<Icon>> &&frames, int framerate_ms)
    : frames(std::move(frames))
  {
    this->framerate_ms = framerate_ms;
    this->num_frames = this->frames.size();
    this->updated_at = std::chrono::system_clock::now();
  }

  void Animation::tick()
  {
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->updated_at);

    if (diff.count() < this->framerate_ms)
      return;

    if (++this->current_frame >= this->num_frames)
      this->current_frame = 0;

    this->updated_at = now;
  }

  void Animation::add(std::unique_ptr<Icon> &&frame)
  {
    this->frames.emplace_back(std::move(frame));
    this->num_frames = this->frames.size();
  }

  std::unique_ptr<Icon> &Animation::get()
  {
    this->tick();
    return this->frames[this->current_frame];
  }

  int Animation::get_framerate() {
    return this->framerate_ms;
  }
}
