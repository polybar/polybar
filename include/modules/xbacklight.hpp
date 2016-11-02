#pragma once

#include "components/config.hpp"
#include "x11/connection.hpp"
#include "x11/randr.hpp"
#include "x11/xutils.hpp"
#include "config.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta.hpp"
#include "utils/throttle.hpp"

LEMONBUDDY_NS

namespace modules {
  /**
   * Backlight module built using the RandR X extension.
   *
   * This is built as a replacement for the old backlight
   * module that was set up using with inotify watches listening
   * for changes to the raw file handle.
   *
   * This module is alot faster, it's more responsive and it will
   * be dormant until new values are reported. Inotify watches
   * are a bit random when it comes to proc-/sysfs.
   *
   * TODO: Implement backlight configuring using scroll events
   */
  class xbacklight_module : public static_module<xbacklight_module>,
                            public xpp::event::sink<evt::randr_notify> {
   public:
    using static_module::static_module;

    void setup();
    void handle(const evt::randr_notify& evt);
    void update();
    bool build(builder* builder, string tag) const;

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR = "<bar>";
    static constexpr auto TAG_RAMP = "<ramp>";

    throttle_util::throttle_t m_throttler;
    connection& m_connection{configure_connection().create<connection&>()};
    monitor_t m_output;

    ramp_t m_ramp;
    label_t m_label;
    progressbar_t m_progressbar;

    int m_percentage = 0;
  };
}

LEMONBUDDY_NS_END
