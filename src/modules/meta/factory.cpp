#include "modules/meta/factory.hpp"

#include "modules/meta/all.hpp"

POLYBAR_NS

namespace modules {

  /**
   * Function pointer for creating a module.
   */
  using factory_fun = module_t (*)(const bar_settings&, string&&);
  using factory_map = map<string, factory_fun>;

  /**
   * Creates a factory function for constructing a module.
   *
   * @tparam M name of the module class
   */
  template <typename M>
  static constexpr factory_fun get_factory() {
    return [](const bar_settings& bar, string&& module_name) -> module_t {
      return make_shared<M>(bar, move(module_name));
    };
  }

  /**
   * Creates an entry for the factories map.
   *
   * Each entry is a pair containing the module type and the factory function.
   *
   * @tparam M name of the module class
   */
  template <typename M>
  static factory_map::value_type map_entry() {
    return std::make_pair(std::string(M::TYPE), get_factory<M>());
  }

  /**
   * Factory function for each module type.
   */
  static const factory_map factories = {
      map_entry<counter_module>(),
      map_entry<backlight_module>(),
      map_entry<battery_module>(),
      map_entry<bspwm_module>(),
      map_entry<cpu_module>(),
      map_entry<date_module>(),
      map_entry<github_module>(),
      map_entry<fs_module>(),
      map_entry<memory_module>(),
      map_entry<i3_module>(),
      map_entry<mpd_module>(),
      map_entry<alsa_module>(),
      map_entry<pulseaudio_module>(),
      map_entry<network_module>(),
#if DEBUG
      map_entry<systray_module>(),
#endif
      map_entry<temperature_module>(),
      map_entry<xbacklight_module>(),
      map_entry<xkeyboard_module>(),
      map_entry<xwindow_module>(),
      map_entry<xworkspaces_module>(),
      map_entry<tray_module>(),
      map_entry<text_module>(),
      map_entry<script_module>(),
      map_entry<menu_module>(),
      map_entry<ipc_module>(),
  };

  module_t make_module(string&& type, const bar_settings& bar, string module_name, const logger& log) {
    string actual_type = type;

    if (type == "internal/volume") {
      log.warn("internal/volume is deprecated, use %s instead", string(alsa_module::TYPE));
      actual_type = alsa_module::TYPE;
    }

    auto it = factories.find(actual_type);
    if (it != factories.end()) {
      return it->second(bar, std::move(module_name));
    } else {
      throw application_error("Unknown module: " + type);
    }
  }
} // namespace modules

POLYBAR_NS_END
