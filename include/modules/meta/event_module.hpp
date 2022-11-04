#pragma once

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  template <class Impl>
  class event_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() override {
      this->module<Impl>::start();
      this->m_mainthread = thread(&event_module::runner, this);
    }

   protected:
    void runner() {
      this->m_log.trace("%s: Thread id = %i", this->name(), concurrency_util::thread_id(this_thread::get_id()));
      try {
        // warm up module output before entering the loop
        std::unique_lock<std::mutex> guard(this->m_updatelock);
        CAST_MOD(Impl)->update();
        CAST_MOD(Impl)->broadcast();
        guard.unlock();

        const auto check = [&]() -> bool {
          std::lock_guard<std::mutex> guard(this->m_updatelock);
          return CAST_MOD(Impl)->has_event() && CAST_MOD(Impl)->update();
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
  };
}  // namespace modules

POLYBAR_NS_END
