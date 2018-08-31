#include "modules/meta/factory.hpp"

POLYBAR_NS

namespace modules {

  factory_map& get_factory_map() {
    static factory_map fmap;
    return fmap;
  }

  module_interface* make_module(string&& name, const bar_settings& bar, string module_name) {
    const auto& fmap = get_factory_map();
    auto it = fmap.find(name);
    if (it != fmap.end()) {
      return it->second(bar, move(module_name));
    } else {
      throw application_error("Unknown module: " + name);
    }
  }

}  // namespace modules

POLYBAR_NS_END
