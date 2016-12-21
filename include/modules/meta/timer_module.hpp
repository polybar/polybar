#pragma once

#include <chrono>

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace chrono = std::chrono;

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
    interval_t m_interval{1};

    void runner() {
      try {
        while (CONST_MOD(Impl).running()) {
          {
            std::lock_guard<std::mutex> guard(this->m_updatelock);

            if (CAST_MOD(Impl)->update())
              this->broadcast();
          }
          if (CONST_MOD(Impl).running()) {
            this->sleep(m_interval);
          }
        }
      } catch (const module_error& err) {
        this->halt(err.what());
      } catch (const std::exception& err) {
        this->halt(err.what());
      }
    }
  };
}

POLYBAR_NS_END
