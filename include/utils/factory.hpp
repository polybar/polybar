#pragma once

#include <unistd.h>

#include "common.hpp"

POLYBAR_NS

namespace factory_util {
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
    static auto instance = make_shared<T>(forward<Deps>(deps)...);
    return instance;
  }
}

POLYBAR_NS_END
