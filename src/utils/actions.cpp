#include "utils/actions.hpp"

#include "common.hpp"

POLYBAR_NS

namespace actions_util {
  route::route() : valid(false), handler_name(""), action("") {}
  route::route(string handler_name, string action) : valid(true), handler_name(handler_name), action(action) {}
  route::route(const modules::input_handler& handler, string action)
      : valid(true), handler_name(handler.input_handler_name()), action(action) {}

  string route::get_action_string() const {
    if (!this->valid) {
      return "";
    }

    return "#" + this->handler_name + "#" + this->action;
  }

  string get_action_string(const modules::input_handler& handler, string action) {
    return route(handler, action).get_action_string();
  }
}  // namespace actions_util

POLYBAR_NS_END
