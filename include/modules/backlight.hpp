#ifndef _MODULES_BACKLIGHT_HPP_
#define _MODULES_BACKLIGHT_HPP_

#include "config.hpp"
#include "modules/base.hpp"
#include "drawtypes/bar.hpp"
#include "drawtypes/label.hpp"

namespace modules
{
  DefineModule(BacklightModule, InotifyModule)
  {
    const char *TAG_LABEL = "<label>";
    const char *TAG_BAR = "<bar>";
    const char *TAG_RAMP = "<ramp>";

    std::unique_ptr<drawtypes::Bar> bar;
    std::unique_ptr<drawtypes::Ramp> ramp;
    std::unique_ptr<drawtypes::Label> label;
    std::unique_ptr<drawtypes::Label> label_tokenized;

    std::string path_val, path_max;
    float val = 0, max = 0;

    std::atomic<int> percentage;

    public:
      BacklightModule(const std::string& name);

      bool on_event(InotifyEvent *event);
      bool build(Builder *builder, const std::string& tag);
  };
}

#endif
