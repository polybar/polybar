#pragma once

#include "common.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  using module_t = shared_ptr<module_interface>;

  /**
   * Creates a new module instance.
   *
   * @param type The type of the module (as given by each module's TYPE field)
   * @param bar An instance of the @ref bar_settings
   * @param module_name The user-specified module name
   * @param log A @ref logger instance
   * @param config A @ref config instance
   */
  module_t make_module(string&& type, const bar_settings& bar, string module_name, const logger& log, const config& config);
} // namespace modules

POLYBAR_NS_END
