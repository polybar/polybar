#include "utils/action_router.hpp"

POLYBAR_NS

action_router::entry::entry(callback func) : without(func), with_data(false){}
action_router::entry::entry(callback_data func) : with(func), with_data(true){}
action_router::entry::~entry() {
  if (with_data) {
    with.~function();
  } else {
    without.~function();
  }
}

void action_router::register_action(const string& name, callback func) {
  register_entry(name, func);
}

void action_router::register_action_with_data(const string& name, callback_data func) {
  register_entry(name, func);
}

bool action_router::has_action(const string& name) {
  return callbacks.find(name) != callbacks.end();
}

/**
 * Invokes the given action name on the passed module pointer.
 *
 * The action must exist.
 */
void action_router::invoke(const string& name, const string& data) {
  auto it = callbacks.find(name);
  assert(it != callbacks.end());

  auto& e = it->second;

  if (e.with_data) {
    e.with(data);
  } else {
    e.without();
  }
}

POLYBAR_NS_END
