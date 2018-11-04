#pragma once

// TODO: move to functional.hpp

#include "common.hpp"

#include "components/logger.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

namespace scope_util {
  template <typename... Args>
  class on_exit {
   public:
    on_exit(function<void(Args...)>&& fn, Args... args) : m_callback(bind(fn, args...)) {}

    virtual ~on_exit() {
      m_callback();
    }

   protected:
    function<void()> m_callback;
  };

  /**
   * Creates a wrapper that will trigger given callback when
   * leaving the object's scope (i.e, when it gets destroyed)
   *
   * Example usage:
   * \code cpp
   *   {
   *     auto handler = scope_util::make_exit_handler([]{ ... })
   *     ...
   *   }
   * \endcode
   */
  template <typename Fn = function<void()>, typename... Args>
  decltype(auto) make_exit_handler(Fn&& fn, Args&&... args) {
    return factory_util::unique<on_exit<Args...>>(forward<Fn>(fn), forward<Args>(args)...);
  }
}

POLYBAR_NS_END
