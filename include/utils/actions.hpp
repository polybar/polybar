#pragma once

#include "common.hpp"
#include "modules/meta/input_handler.hpp"

POLYBAR_NS

namespace actions_util {
  /**
   * Specifies how an action is routed
   *
   * A route consists of an input handler where the action should be delivered
   * and the action itself.
   *
   * TODO maybe remove if redundant at the end
   */
  struct route {
    bool valid;
    string handler_name;
    string action;

    explicit route();
    explicit route(string handler_name, string action);
    explicit route(const modules::input_handler& handler, string action);

    /**
     * Constructs the full action string for this route
     */
    string get_action_string() const;
  };

  string get_action_string(const modules::input_handler& handler, string action);
}  // namespace actions_util

POLYBAR_NS_END
