#pragma once

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  template <class Impl>
  class static_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() {
      this->m_mainthread = thread([&] {
        this->m_log.trace("%s: Thread id = %i", this->name(), concurrency_util::thread_id(this_thread::get_id()));
        CAST_MOD(Impl)->update();
        CAST_MOD(Impl)->broadcast();
      });
    }

    bool build(builder*, string) const {
      return true;
    }
  };
}

POLYBAR_NS_END
