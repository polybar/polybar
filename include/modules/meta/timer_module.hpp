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
          // wait until next full interval to avoid drifting clocks
          using clock = chrono::system_clock;
          using sys_duration_t = clock::time_point::duration;

          auto sys_interval = chrono::duration_cast<sys_duration_t>(m_interval);
          clock::time_point now = clock::now();
          sys_duration_t adjusted = sys_interval - (now.time_since_epoch() % sys_interval);

          // The seemingly arbitrary addition of 500ms is due
          // to the fact that if we wait the exact time our
          // thread will be woken just a tiny bit prematurely
          // and therefore the wrong time will be displayed.
          // It is currently unknown why exactly the thread gets
          // woken prematurely.
          CAST_MOD(Impl)->sleep_until(now + adjusted + 500ms);
        }
      } catch (const exception& err) {
        CAST_MOD(Impl)->halt(err.what());
      }
    }

   protected:
    interval_t m_interval{1.0};
  };
}  // namespace modules

POLYBAR_NS_END
