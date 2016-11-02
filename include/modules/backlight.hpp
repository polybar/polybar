#pragma once

#include "components/config.hpp"
#include "config.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  struct brightness_handle {
    void filepath(string path);
    float read() const;

   private:
    string m_path;
  };

  class backlight_module : public inotify_module<backlight_module> {
   public:
    using inotify_module::inotify_module;

    void setup();
    void idle();
    bool on_event(inotify_event* event);
    bool build(builder* builder, string tag) const;

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR = "<bar>";
    static constexpr auto TAG_RAMP = "<ramp>";

    ramp_t m_ramp;
    label_t m_label;
    progressbar_t m_progressbar;

    brightness_handle m_val;
    brightness_handle m_max;

    int m_percentage = 0;
  };
}

LEMONBUDDY_NS_END
