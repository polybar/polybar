#pragma once

#include "modules/base.hpp"

namespace modules
{
  DefineModule(CounterModule, TimerModule)
  {
    static constexpr auto TAG_COUNTER = "<counter>";

    concurrency::Atomic<int> counter;

    public:
      explicit CounterModule(std::string name);

      bool update();
      bool build(Builder *builder, std::string tag);
  };
}
