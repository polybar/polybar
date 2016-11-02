#pragma once

#include "modules/meta.hpp"

LEMONBUDDY_NS

using namespace drawtypes;

namespace modules {
  class counter_module : public timer_module<counter_module> {
   public:
    using timer_module::timer_module;

    void setup();
    bool update();
    bool build(builder* builder, string tag) const;

   private:
    static constexpr auto TAG_COUNTER = "<counter>";

    int m_counter{0};
  };
}

LEMONBUDDY_NS_END
