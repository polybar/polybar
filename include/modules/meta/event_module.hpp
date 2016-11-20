#pragma once

// #include "components/types.hpp"
// #include "components/builder.hpp"

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  template <class Impl>
  class event_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start();

   protected:
    void runner();
  };
}

POLYBAR_NS_END
