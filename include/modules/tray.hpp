#pragma once

#include "common.hpp"
#include "components/bar.hpp"
#include "modules/meta/static_module.hpp"
#include "x11/tray_manager.hpp"

POLYBAR_NS
namespace modules {
class tray_module : public static_module<tray_module> {
 public:
  explicit tray_module(const bar_settings& bar_settings, string name_, const config&);
  string get_format() const;

  void start() override;

  bool build(builder* builder, const string& tag) const;
  void update() {}

  static constexpr auto TYPE = "internal/tray";

 private:
  static constexpr const char* TAG_TRAY{"<tray>"};

  tray::manager m_tray;
};
} // namespace modules
POLYBAR_NS_END
