#pragma once

#include "common.hpp"
#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {

  using factory_function = module_interface* (*)(const bar_settings&, string, const logger&);
  using factory_map = std::unordered_map<string, factory_function>;
  factory_map& get_factory_map();

  template <class Module>
  class module_registration {
   public:
    module_registration(string name, factory_function ff) {
      get_factory_map()[std::move(name)] = ff;
    }
  };

  module_interface* make_module(string&& name, const bar_settings& bar, string module_name, const logger& m_log);
}  // namespace modules

#define POLYBAR_MODULE(MODULE_TYPE, MODULE_NAME)                                                            \
  module_interface* MODULE_TYPE##_create(const bar_settings& bar, std::string module_name, const logger&) { \
    return new MODULE_TYPE(bar, std::move(module_name));                                                    \
  }                                                                                                         \
  module_registration<MODULE_TYPE> MODULE_TYPE##_registration(MODULE_NAME, &MODULE_TYPE##_create)

POLYBAR_NS_END
