#pragma once

#include "common.hpp"
#include "modules/meta/input_handler.hpp"

POLYBAR_NS

namespace actions_util {
  string get_action_string(const modules::input_handler& handler, string action, string data);
}  // namespace actions_util

POLYBAR_NS_END
