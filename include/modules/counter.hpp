#pragma once

#include "modules/meta/timer_module.hpp"

POLYBAR_NS

namespace modules {
  class counter_module : public timer_module<counter_module> {
   public:
    explicit counter_module(const bar_settings&, string);

    bool update();
    bool build(builder* builder, const string& tag) const;

   private:
    static constexpr auto TAG_COUNTER = "<counter>";

    int m_counter{0};
  };
}

POLYBAR_NS_END
