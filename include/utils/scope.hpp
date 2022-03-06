#pragma once

#include "common.hpp"

POLYBAR_NS

namespace scope_util {
  /**
   * Creates a wrapper that will trigger given callback when
   * leaving the object's scope (i.e, when it gets destroyed)
   *
   * Example usage:
   * @code cpp
   *   {
   *     on_exit handler([]{ ... });
   *     ...
   *   }
   * @endcode
   */
  class on_exit {
   public:
    on_exit(const function<void(void)>& fn) : m_callback(fn) {}

    virtual ~on_exit() {
      m_callback();
    }

   protected:
    function<void(void)> m_callback;
  };
} // namespace scope_util

POLYBAR_NS_END
