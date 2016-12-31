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
      try {
        while (this->running()) {
          std::unique_lock<std::mutex> guard(this->m_updatelock);

          if (CAST_MOD(Impl)->update()) {
            this->broadcast();
          }

          if (this->running()) {
            guard.unlock();
            this->sleep(m_interval);
          }
        }
      } catch (const exception& err) {
        this->halt(err.what());
      }
    }

   protected:
    interval_t m_interval{1.0};
  };
}

POLYBAR_NS_END
