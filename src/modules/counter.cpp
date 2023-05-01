#include "modules/counter.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<counter_module>;

  counter_module::counter_module(const bar_settings& bar, string name_, const config& config)
      : timer_module<counter_module>(bar, move(name_), config) {
    set_interval(1s);
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
}  // namespace modules

POLYBAR_NS_END
