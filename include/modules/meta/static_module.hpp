#pragma once

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  /**
   * \brief Generic class for a static module.
   * \details
   * To implement this module, the following method should be implemented:
   *   - update() : CRTP implementation
   * \see module
   */
  template <class Impl>
  class static_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() override {
      {
        std::lock_guard<std::mutex> guard(this->m_modulelock);
        CAST_MOD(Impl)->update();
      }
      CAST_MOD(Impl)->broadcast();
    }

    bool build(builder*, const string&) const {
      return true;
    }

   protected:
    /**
     * \brief Updates the internal state of the module and returns true if any modification has been made.
     * \details
     * Contract:
     *   - expects: the mutex #m_modulelock is locked
     *   - ensures: the mutex #m_modulelock is still locked.
     */
    void update() = delete;
  };
}  // namespace modules

POLYBAR_NS_END
