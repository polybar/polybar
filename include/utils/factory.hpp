#pragma once

#include <unistd.h>

#include "common.hpp"

POLYBAR_NS

namespace factory_util {
  namespace detail {
    struct null_deleter {
      template <typename T>
      void operator()(T*) const {}
    };
  }  // namespace detail

  extern detail::null_deleter null_deleter;

  template <class T, class... Deps>
  shared_ptr<T> singleton(Deps&&... deps) {
    static shared_ptr<T> instance{make_shared<T>(forward<Deps>(deps)...)};
    return instance;
  }
}  // namespace factory_util

POLYBAR_NS_END
