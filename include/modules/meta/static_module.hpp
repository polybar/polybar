#pragma once

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  /**
   * @brief Generic class for a static module.
   * @details
   * To implement this module, the following method should be implemented:
   *   - update() : CRTP implementation
   * @see module
   */
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
}  // namespace modules

POLYBAR_NS_END
