#pragma once

#include <cassert>
#include <stdexcept>
#include <unordered_map>

#include "common.hpp"

POLYBAR_NS

/**
 * Maps action names to function pointers in this module and invokes them.
 *
 * Each module has one instance of this class and uses it to register action.
 * For each action the module has to register the name, whether it can take
 * additional data, and a pointer to the member function implementing that
 * action.
 *
 * Ref: https://isocpp.org/wiki/faq/pointers-to-members
 *
 * The input() function in the base class uses this for invoking the actions
 * of that module.
 *
 * Any module that does not reimplement that function will automatically use
 * this class for action routing.
 */
template <typename Impl>
class action_router {
  typedef void (Impl::*callback)();
  typedef void (Impl::*callback_data)(const std::string&);

 public:
  explicit action_router(Impl* This) : m_this(This) {}

  void register_action(const string& name, callback func) {
    entry e;
    e.with_data = false;
    e.without = func;
    register_entry(name, e);
  }

  void register_action_with_data(const string& name, callback_data func) {
    entry e;
    e.with_data = true;
    e.with = func;
    register_entry(name, e);
  }

  bool has_action(const string& name) {
    return callbacks.find(name) != callbacks.end();
  }

  /**
   * Invokes the given action name on the passed module pointer.
   *
   * The action must exist.
   */
  void invoke(const string& name, const string& data) {
    auto it = callbacks.find(name);
    assert(it != callbacks.end());

    entry e = it->second;

#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))
    if (e.with_data) {
      CALL_MEMBER_FN(*m_this, e.with)(data);
    } else {
      CALL_MEMBER_FN(*m_this, e.without)();
    }
#undef CALL_MEMBER_FN
  }

 protected:
  struct entry {
    union {
      callback without;
      callback_data with;
    };
    bool with_data;
  };

  void register_entry(const string& name, const entry& e) {
    if (has_action(name)) {
      throw std::invalid_argument("Tried to register action '" + name + "' twice. THIS IS A BUG!");
    }
    callbacks[name] = e;
  }

 private:
  std::unordered_map<string, entry> callbacks;
  Impl* m_this;
};

POLYBAR_NS_END
