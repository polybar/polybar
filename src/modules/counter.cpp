#include "modules/counter.hpp"

#include "modules/meta/base.inl"
#include "modules/meta/timer_module.inl"

POLYBAR_NS

namespace modules {
  template class module<counter_module>;
  template class timer_module<counter_module>;

  void counter_module::setup() {
    m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 1));
    m_formatter->add(DEFAULT_FORMAT, TAG_COUNTER, {TAG_COUNTER});
  }

  bool counter_module::update() {
    m_counter++;
    return true;
  }

  bool counter_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_COUNTER) {
      builder->node(to_string(m_counter));
      return true;
    }
    return false;
  }
}

POLYBAR_NS_END
