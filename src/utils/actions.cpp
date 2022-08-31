#include "utils/actions.hpp"

#include <cassert>
#include <stdexcept>

#include "common.hpp"
#include "modules/meta/base.hpp"

POLYBAR_NS

namespace actions_util {
  string get_action_string(const modules::module_interface& module, string action, string data) {
    return get_action_string(module.name_raw(), action, data);
  }

  string get_action_string(const string& module_name, string action, string data) {
    string str = "#" + module_name + "." + action;
    if (!data.empty()) {
      str += "." + data;
    }

    return str;
  }

  std::tuple<string, string, string> parse_action_string(string action_str) {
    assert(action_str.front() == '#');

    action_str.erase(0, 1);

    auto action_sep = action_str.find('.');

    if (action_sep == string::npos) {
      throw std::runtime_error("Missing separator between name and action");
    }

    auto module_name = action_str.substr(0, action_sep);

    if (module_name.empty()) {
      throw std::runtime_error("The module name must not be empty");
    }

    auto action = action_str.substr(action_sep + 1);
    auto data_sep = action.find('.');
    string data;

    if (data_sep != string::npos) {
      data = action.substr(data_sep + 1);
      action.erase(data_sep);
    }

    if (action.empty()) {
      throw std::runtime_error("The action name must not be empty");
    }

    return std::tuple<string, string, string>{module_name, action, data};
  }
}  // namespace actions_util

POLYBAR_NS_END
