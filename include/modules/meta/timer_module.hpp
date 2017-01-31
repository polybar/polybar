#pragma once

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  using interval_t = chrono::duration<double>;

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
        std::unique_lock<std::mutex> guard(this->m_updatelock);
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

   protected:
    interval_t m_interval{1.0};
  };
}

POLYBAR_NS_END
