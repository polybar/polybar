#pragma once

// #include "components/builder.hpp"
// #include "components/types.hpp"

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  template <class Impl>
  class static_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start();
    bool build(builder*, string) const;
  };
}

POLYBAR_NS_END
