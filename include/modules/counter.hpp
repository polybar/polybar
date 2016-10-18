#pragma once

#include "modules/meta.hpp"

LEMONBUDDY_NS

using namespace drawtypes;

namespace modules {
  class counter_module : public timer_module<counter_module> {
   public:
    using timer_module::timer_module;

    void setup() {
      m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 1));
      m_formatter->add(DEFAULT_FORMAT, TAG_COUNTER, {TAG_COUNTER});
    }

    bool update() {
      m_counter++;
      return true;
    }

    bool build(builder* builder, string tag) const {
      if (tag == TAG_COUNTER) {
        builder->node(to_string(m_counter));
        return true;
      }
      return false;
    }

   private:
    static constexpr auto TAG_COUNTER = "<counter>";

    int m_counter{0};
  };
}

LEMONBUDDY_NS_END
