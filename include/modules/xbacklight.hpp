#pragma once

#include "components/config.hpp"
#include "modules/meta/event_handler.hpp"
#include "modules/meta/static_module.hpp"
#include "modules/meta/types.hpp"
#include "settings.hpp"
#include "x11/extensions/randr.hpp"

POLYBAR_NS

class connection;

namespace modules {
  /**
   * Backlight module built using the RandR X extension.
   *
   * This is built as a replacement for the old backlight
   * module that was set up using with inotify watches listening
   * for changes to the raw file handle.
   *
   * This module is a lot faster, it's more responsive and it will
   * be dormant until new values are reported. Inotify watches
   * are a bit random when it comes to proc-/sysfs.
   */
  class xbacklight_module : public static_module<xbacklight_module>, public event_handler<evt::randr_notify> {
   public:
    explicit xbacklight_module(const bar_settings& bar, string name_, const config&);

    void update();
    string get_output();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = XBACKLIGHT_TYPE;

    static constexpr const char* EVENT_INC = "inc";
    static constexpr const char* EVENT_DEC = "dec";

   protected:
    void handle(const evt::randr_notify& evt) override;

    void action_inc();
    void action_dec();

    void change_value(int value_mod);

   private:
    static constexpr const char* TAG_LABEL{"<label>"};
    static constexpr const char* TAG_BAR{"<bar>"};
    static constexpr const char* TAG_RAMP{"<ramp>"};

    connection& m_connection;
    monitor_t m_output;
    xcb_window_t m_proxy{};

    ramp_t m_ramp;
    label_t m_label;
    progressbar_t m_progressbar;

    bool m_scroll{true};
    int m_percentage{0};
  };
}  // namespace modules

POLYBAR_NS_END
