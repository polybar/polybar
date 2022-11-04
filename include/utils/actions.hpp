#pragma once

#include "common.hpp"

namespace modules {
  struct module_interface;
}  // namespace modules

POLYBAR_NS

namespace actions_util {

  using action = std::tuple<string, string, string>;

  string get_action_string(const modules::module_interface& module, string action, string data);
  string get_action_string(const string& module_name, string action, string data);

  /**
   * Parses an action string of the form "#name.action[.data]".
   *
   * Only call this function with an action string that begins with '#'.
   *
   * @returns a triple (name, action, data)
   *          If no data exists, the third string will be empty.
   *          This means "#name.action." and "#name.action" will be produce the
   *          same result.
   * @throws runtime_error If the action string is malformed
   */
  action parse_action_string(string action);
}  // namespace actions_util

POLYBAR_NS_END
