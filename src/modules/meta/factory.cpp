#include "modules/meta/factory.hpp"

#include "modules/meta/all.hpp"

POLYBAR_NS

namespace modules {

  /**
   * Function pointer for creating a module.
   */
  using factory_fun = module_t (*)(const bar_settings&, string&&, const config&);
  using factory_map = map<string, factory_fun>;

  /**
   * Creates a factory function for constructing a module.
   *
   * @tparam M name of the module class
   */
  template <typename M>
  static constexpr factory_fun get_factory() {
    return [](const bar_settings& bar, string&& module_name, const config& config) -> module_t {
      return make_shared<M>(bar, move(module_name), config);
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

  template<const char* module_type>
  static factory_map::value_type map_entry_unsupported() {
    return {
      module_type, 
      [](const bar_settings&, string&&, const config&) -> module_t {
        throw application_error("No built-in support for '" + string(module_type) + "'");
      }
    };
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
#if ENABLE_CURL
      map_entry<github_module>(),
#else
      map_entry_unsupported<GITHUB_TYPE>(),
#endif
      map_entry<fs_module>(),
      map_entry<memory_module>(),
#if ENABLE_I3
      map_entry<i3_module>(),
#else
      map_entry_unsupported<I3_TYPE>(),
#endif
#if ENABLE_MPD
      map_entry<mpd_module>(),
#else
      map_entry_unsupported<MPD_TYPE>(),
#endif
#if ENABLE_ALSA
      map_entry<alsa_module>(),
#else
      map_entry_unsupported<ALSA_TYPE>(),
#endif
#if ENABLE_PULSEAUDIO
      map_entry<pulseaudio_module>(),
#else
      map_entry_unsupported<PULSEAUDIO_TYPE>(),
#endif
#if ENABLE_NETWORK
      map_entry<network_module>(),
#else
      map_entry_unsupported<NETWORK_TYPE>(),
#endif
      map_entry<temperature_module>(),
      map_entry<xbacklight_module>(),
#if ENABLE_XKEYBOARD
      map_entry<xkeyboard_module>(),
#else
      map_entry_unsupported<XKEYBOARD_TYPE>(),
#endif
      map_entry<xwindow_module>(),
      map_entry<xworkspaces_module>(),
      map_entry<tray_module>(),
      map_entry<text_module>(),
      map_entry<script_module>(),
      map_entry<menu_module>(),
      map_entry<ipc_module>(),
  };

  module_t make_module(string&& type, const bar_settings& bar, string module_name, const logger& log, const config& config) {
    string actual_type = type;

    if (type == "internal/volume") {
      log.warn("internal/volume is deprecated, use %s instead", ALSA_TYPE);
      actual_type = ALSA_TYPE;
    }

    auto it = factories.find(actual_type);
    if (it != factories.end()) {
      return it->second(bar, std::move(module_name), config);
    } else {
      throw application_error("Unknown module: " + type);
    }
  }
} // namespace modules

POLYBAR_NS_END
