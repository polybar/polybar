#pragma once

#include "components/config.hpp"
#include "settings.hpp"
#include "modules/meta/inotify_module.hpp"

POLYBAR_NS

namespace modules {
  class backlight_module : public inotify_module<backlight_module> {
   public:
    struct brightness_handle {
      void filepath(const string& path);
      float read() const;

     private:
      string m_path;
    };

   public:
    explicit backlight_module(const bar_settings&, string);

    void idle();
    bool on_event(inotify_event* event);
    bool build(builder* builder, const string& tag) const;

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

POLYBAR_NS_END
