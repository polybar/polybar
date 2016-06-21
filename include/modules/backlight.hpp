#pragma once

#include "config.hpp"
#include "modules/base.hpp"
#include "drawtypes/bar.hpp"
#include "drawtypes/label.hpp"

namespace modules
{
  DefineModule(BacklightModule, InotifyModule)
  {
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR = "<bar>";
    static constexpr auto TAG_RAMP = "<ramp>";

    std::unique_ptr<drawtypes::Bar> bar;
    std::unique_ptr<drawtypes::Ramp> ramp;
    std::unique_ptr<drawtypes::Label> label;
    std::unique_ptr<drawtypes::Label> label_tokenized;

    std::string path_val, path_max;
    float val = 0, max = 0;

    concurrency::Atomic<int> percentage;

    public:
      explicit BacklightModule(std::string name);

      bool on_event(InotifyEvent *event);
      bool build(Builder *builder, std::string tag);

      void idle() const
      {
        std::this_thread::sleep_for(25ms);
      }
  };
}
