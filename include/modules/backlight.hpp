#pragma once

#include "components/config.hpp"
#include "modules/meta/inotify_module.hpp"
#include "settings.hpp"

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

    string get_output();

   public:
    explicit backlight_module(const bar_settings&, string);

    void idle();
    bool on_event(const inotify_event& event);
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = "internal/backlight";

    static constexpr const char* EVENT_INC = "inc";
    static constexpr const char* EVENT_DEC = "dec";

   protected:
    void action_inc();
    void action_dec();

    void change_value(int value_mod);

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR = "<bar>";
    static constexpr auto TAG_RAMP = "<ramp>";

    ramp_t m_ramp;
    label_t m_label;
    progressbar_t m_progressbar;
    string m_path_backlight;
    float m_max_brightness;
    bool m_scroll{false};
    bool m_use_actual_brightness{true};

    brightness_handle m_val;
    brightness_handle m_max;

    int m_percentage = 0;
  };
} // namespace modules

POLYBAR_NS_END
