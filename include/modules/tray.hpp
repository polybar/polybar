#pragma once

#include "common.hpp"
#include "components/bar.hpp"
#include "modules/meta/static_module.hpp"

POLYBAR_NS
namespace modules {
  class tray_module : public static_module<tray_module>,
                      public signal_receiver<SIGN_PRIORITY_TRAY, signals::ui_tray::tray_width_change> {
   public:
    explicit tray_module(const bar_settings& bar_settings, string name_);
    string get_format() const;

    bool build(builder* builder, const string& tag) const;
    void update() {}
    void teardown();

    bool on(const signals::ui_tray::tray_width_change& evt) override;

    static constexpr auto TYPE = "internal/tray";

   private:
    static constexpr const char* TAG_TRAY{"<tray>"};

    int m_width{0};
  };
}  // namespace modules
POLYBAR_NS_END
