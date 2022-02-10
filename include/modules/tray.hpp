#pragma once

#include "common.hpp"
#include "modules/meta/event_module.hpp"
#include "components/bar.hpp"

POLYBAR_NS
namespace modules {
  class tray_module : public event_module<tray_module>, signal_receiver<SIGN_PRIORITY_TRAY,signals::ui_tray::tray_width_change>{
   public:
    explicit tray_module(const bar_settings& bar_settings, string name_);
    string get_output();
    string get_format() const;

    bool has_event();
    bool update();
    bool build(builder* builder, const string& tag) const;

    bool on(const signals::ui_tray::tray_width_change& evt) override;

    static constexpr auto TYPE = "internal/tray";

   private:
    int width;
    bool toUpdate;
  };
}  // namespace modules
POLYBAR_NS_END
