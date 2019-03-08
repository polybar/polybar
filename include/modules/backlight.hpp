#pragma once

#include "components/config.hpp"
#include "modules/meta/udev_module.hpp"
#include "settings.hpp"

POLYBAR_NS

namespace modules {
  class backlight_module : public udev_module<backlight_module> {
   public:
    explicit backlight_module(const bar_settings&, string);

    void idle();
    bool on_event(udev_event&& event);
    bool build(builder* builder, const string& tag) const;

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR = "<bar>";
    static constexpr auto TAG_RAMP = "<ramp>";

    ramp_t m_ramp;
    label_t m_label;
    progressbar_t m_progressbar;

    string m_card;

    int m_percentage = 0;
  };
}  // namespace modules

POLYBAR_NS_END
