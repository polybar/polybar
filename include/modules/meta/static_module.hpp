#pragma once

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  template <class Impl>
  class static_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() override {
      this->module<Impl>::start();
      CAST_MOD(Impl)->update();
      CAST_MOD(Impl)->broadcast();
    }

    bool build(builder*, string) const {
      return true;
    }
  };
}  // namespace modules

POLYBAR_NS_END
