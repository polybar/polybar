#include "config.hpp"
#include "modules/counter.hpp"
#include "utils/config.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"

using namespace modules;

CounterModule::CounterModule(const std::string& module_name) : TimerModule(module_name, 1s)
{
  this->interval = std::chrono::duration<double>(
    config::get<float>(name(), "interval", 1));

  this->formatter->add(DEFAULT_FORMAT, TAG_COUNTER, { TAG_COUNTER });

  this->counter = 0;
}

bool CounterModule::update()
{
  this->counter = this->counter() + 1;
  return true;
}

bool CounterModule::build(Builder *builder, const std::string& tag)
{
  if (tag == TAG_COUNTER) {
    builder->node(std::to_string(this->counter()));
    return true;
  }

  return false;
}
