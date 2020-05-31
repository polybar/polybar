#pragma once

#include "common.hpp"
#include "modules/meta/input_handler.hpp"

POLYBAR_NS

namespace actions_util {
  string get_action_string(const modules::input_handler& handler, string action, string data);

  /**
   * Parses an action string of the form "#name.action[.data]".
   *
   * Only call this function with an action string that begins with '#'.
   *
   * \returns a triple (name, action, data)
   *          If no data exists, the third string will be empty.
   *          This means "#name.action." and "#name.action" will be produce the
   *          same result.
   * \throws runtime_error If the action string is malformed
   */
  std::tuple<string, string, string> parse_action_string(string action);
}  // namespace actions_util

POLYBAR_NS_END
