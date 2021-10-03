#pragma once

#include <cassert>
#include <stdexcept>
#include <unordered_map>

#include "common.hpp"

POLYBAR_NS

/**
 * Maps action names to lambdas and invokes them.
 *
 * Each module has one instance of this class and uses it to register action.
 * For each action the module has to register the name, whether it can take
 * additional data, and a callback that is called whenever the action is triggered.
 *
 * The input() function in the base class uses this for invoking the actions
 * of that module.
 *
 * Any module that does not reimplement that function will automatically use
 * this class for action routing.
 */
class action_router {
  using callback = std::function<void(void)>;
  using callback_data = std::function<void(const std::string&)>;

 public:
  void register_action(const string& name, callback func);
  void register_action_with_data(const string& name, callback_data func);
  bool has_action(const string& name);
  void invoke(const string& name, const string& data);

 protected:
  struct entry {
    union {
      callback without;
      callback_data with;
    };
    bool with_data;

    entry(callback func);
    entry(callback_data func);
    ~entry();
  };

  template <typename F>
  void register_entry(const string& name, const F& e) {
    if (has_action(name)) {
      throw std::invalid_argument("Tried to register action '" + name + "' twice. THIS IS A BUG!");
    }

    callbacks.emplace(name, std::move(e));
  }

 private:
  std::unordered_map<string, entry> callbacks;
};

POLYBAR_NS_END
