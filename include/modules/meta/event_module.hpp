#pragma once

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  /**
   * Generic class to have a module that listen events
   * @details
   * To implement this module, the following methods should be implemented:
   *   - update(): CRTP implementation
   *   - has_event(): CRTP implementation
   *
   * @see module
   */
  template <class Impl>
  class event_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() {
      this->m_mainthread = thread(&event_module::runner, this);
    }

   protected:
    void runner() {
      this->m_log.trace("%s: Thread id = %i", this->name(), concurrency_util::thread_id(this_thread::get_id()));
      try {
        // warm up module output before entering the loop
        {
          std::lock_guard<std::mutex> guard(this->m_modulelock);
          CAST_MOD(Impl)->update();
          CAST_MOD(Impl)->broadcast();
        }

        const auto check = [&]() -> bool {
          if (CAST_MOD(Impl)->has_event()) {
            std::lock_guard<std::mutex> guard(this->m_modulelock);
            return CAST_MOD(Impl)->update();
          }

          return false;
        };

        while (this->running()) {
          if (check()) {
            CAST_MOD(Impl)->broadcast();
          }
          CAST_MOD(Impl)->idle();
        }
      } catch (const exception& err) {
        CAST_MOD(Impl)->halt(err.what());
      }
    }

    /**
     * @brief Returns true if an event should be processed
     * @details
     * This method is NOT protected and may block, however the `m_modulelock` MUST NOT be locked when blocking.
     */
    bool has_event() = delete;

    /**
     * @brief Updates the internal state of the module and returns true if any modification has been made.
     * @details
     * Contract:
     *   - expects: the mutex #m_modulelock is locked
     *   - ensures: the mutex #m_modulelock is still locked.
     */
    bool update() = delete;
  };
}  // namespace modules

POLYBAR_NS_END
