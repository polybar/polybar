#pragma once

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  using interval_t = chrono::duration<double>;

  /**
   * @brief Generic module for modules that need to be updated regularly.
   * @details
   * To implement this module, the following method should be implemented:
   *   - update(): CRTP implementation
   *
   * @see module
   */
  template <class Impl>
  class timer_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() {
      this->m_mainthread = thread(&timer_module::runner, this);
    }

   protected:
    void runner() {
      this->m_log.trace("%s: Thread id = %i", this->name(), concurrency_util::thread_id(this_thread::get_id()));

      const auto check = [&]() -> bool {
        std::lock_guard<std::mutex> guard(this->m_modulelock);
        return CAST_MOD(Impl)->update();
      };

      try {
        // warm up module output before entering the loop
        check();
        CAST_MOD(Impl)->broadcast();

        while (this->running()) {
          if (check()) {
            CAST_MOD(Impl)->broadcast();
          }
          CAST_MOD(Impl)->sleep(m_interval);
        }
      } catch (const exception& err) {
        CAST_MOD(Impl)->halt(err.what());
      }
    }

    /**
     * @brief Updates the internal state of the module and returns true if any modification has been made.
     * @details
     * Contract:
     *   - expects: the mutex #m_modulelock is locked
     *   - ensures: the mutex #m_modulelock is still locked.
     */
    bool update();

   protected:
    interval_t m_interval{1.0};
  };
}

POLYBAR_NS_END
