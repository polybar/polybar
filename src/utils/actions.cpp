#include "utils/actions.hpp"

#include "common.hpp"

POLYBAR_NS

namespace actions_util {
  string get_action_string(const modules::input_handler& handler, string action, string data) {
    string str = "#" + handler.input_handler_name() + "#" + action;
    if (!data.empty()) {
      str += "." + data;
    }

    return str;
  }
}  // namespace actions_util

POLYBAR_NS_END
