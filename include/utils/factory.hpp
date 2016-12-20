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

    struct fd_deleter {
      void operator()(int* fd) const {
        if (fd != nullptr && *fd > 0) {
          close(*fd);
        }
      }
    };
  }

  extern detail::null_deleter null_deleter;
  extern detail::fd_deleter fd_deleter;

  template <typename T, typename... Deps>
  unique_ptr<T> unique(Deps&&... deps) {
    return make_unique<T>(forward<Deps>(deps)...);
  }

  template <typename T, typename... Deps>
  shared_ptr<T> shared(Deps&&... deps) {
    return make_shared<T>(forward<Deps>(deps)...);
  }

  template <class T, class... Deps>
  shared_ptr<T> singleton(Deps&&... deps) {
    static shared_ptr<T> instance{make_shared<T>(forward<Deps>(deps)...)};
    return instance;
  }
}

POLYBAR_NS_END
