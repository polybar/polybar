#pragma once

#include "common.hpp"
#include "components/bar.hpp"
#include "modules/meta/static_module.hpp"
#include "modules/meta/types.hpp"
#include "x11/tray_manager.hpp"

POLYBAR_NS
namespace modules {
/**
 * Wraps the tray_manager in a module.
 *
 * The module produces the `%{Pt}` formatting tag, which is used by the renderer
 * to place the tray.
 * The visibility of the tray icons is directly tied to the visibility of the
 * module.
 */
class tray_module : public static_module<tray_module> {
 public:
  explicit tray_module(const bar_settings& bar_settings, string name_, const config&);
  string get_format() const;

  void set_visible(bool value) override;

  void start() override;

  bool build(builder* builder, const string& tag) const;
  void update() {}

  static constexpr auto TYPE = TRAY_TYPE;

 private:
  static constexpr const char* TAG_TRAY{"<tray>"};

  tray::manager m_tray;
};
} // namespace modules
POLYBAR_NS_END
