#include "modules/counter.hpp"

LEMONBUDDY_NS

namespace modules {
  void counter_module::setup() {
    m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 1));
    m_formatter->add(DEFAULT_FORMAT, TAG_COUNTER, {TAG_COUNTER});
  }

  bool counter_module::update() {
    m_counter++;
    return true;
  }

  bool counter_module::build(builder* builder, string tag) const {
    if (tag == TAG_COUNTER) {
      builder->node(to_string(m_counter));
      return true;
    }
    return false;
  }
}

LEMONBUDDY_NS_END
