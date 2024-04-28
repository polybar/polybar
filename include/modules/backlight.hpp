#pragma once

#include "components/config.hpp"
#include "modules/meta/inotify_module.hpp"
#include "modules/meta/types.hpp"
#include "settings.hpp"

POLYBAR_NS

namespace modules {
  /**
   * Reads value from `/sys/class/backlight/` to get a brightness value for some device.
   *
   * There are two file providing brightness values: `brightness` and `actual_brightness`.
   * The `actual_brightness` file is usually more reliable, but in some cases does not work (provides completely wrong
   * values, doesn't update) depending on kernel version, graphics driver, and/or graphics card.
   * Which file is used is controlled by the use-actual-brightness setting.
   *
   * The general issue with the `brightness` file seems to be that, while it does receive inotify events, the events it
   * receives are not for modification of the file and arrive just before the file is updated with a new value. The module
   * thus reads and displays an outdated brightness value. To compensate for this, the module periodically (controlled by
   * `poll-interval`) forces an update. By default, this is only enabled if the `backlight` file is used.
   */
  class backlight_module : public inotify_module<backlight_module> {
   public:
    struct brightness_handle {
      void filepath(const string& path);
      float read() const;

     private:
      string m_path;
    };

    string get_output();

    explicit backlight_module(const bar_settings&, string, const config&);

    void idle();
    bool on_event(const inotify_event& event);
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = BACKLIGHT_TYPE;

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
    float m_max_brightness{};
    bool m_scroll{false};
    int m_scroll_interval{5};
    bool m_use_actual_brightness{true};

    brightness_handle m_val;
    brightness_handle m_max;

    /**
     * Initial value set to a negative number so that any value read from the backlight file triggers an update during
     * the first read.
     * Otherwise, tokens may not be replaced
     */
    int m_percentage = -1;

    chrono::duration<double> m_interval{};
    chrono::steady_clock::time_point m_lastpoll;
  };
} // namespace modules

POLYBAR_NS_END
