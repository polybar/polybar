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

    void start();

   protected:
    interval_t m_interval{1};

    void runner();
  };
}

POLYBAR_NS_END
