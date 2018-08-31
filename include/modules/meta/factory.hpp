#pragma once

#include "common.hpp"
#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {

  using factory_map = std::unordered_map<string, std::function<module_interface*(const bar_settings&, string)>>;
  factory_map& get_factory_map();

  template <class Module>
  class module_registration {
   public:
    module_registration(string name) {
      get_factory_map()[std::move(name)] = [](const bar_settings& bar, string module_name) {
        return new Module(bar, std::move(module_name));
      };
    }
  };

  module_interface* make_module(string&& name, const bar_settings& bar, string module_name);
}  // namespace modules

#define POLYBAR_MODULE(MODULE_TYPE, MODULE_NAME) \
  module_registration<MODULE_TYPE> MODULE_TYPE ## _registration{MODULE_NAME}

POLYBAR_NS_END
