#ifndef _MODULES_COUNTER_HPP_
#define _MODULES_COUNTER_HPP_

#include "modules/base.hpp"

namespace modules
{
  DefineModule(CounterModule, TimerModule)
  {
    const char *TAG_COUNTER = "<counter>";

    concurrency::Atomic<int> counter;

    public:
      CounterModule(const std::string& name);

      bool update();
      bool build(Builder *builder, const std::string& tag);
  };
}

#endif
