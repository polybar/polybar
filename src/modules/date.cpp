#include "bar.hpp"
#include "lemonbuddy.hpp"
#include "modules/date.hpp"
#include "utils/config.hpp"
#include "utils/string.hpp"

using namespace modules;

DateModule::DateModule(const std::string& name_) : TimerModule(name_, 1s)
{
  this->builder = std::make_unique<Builder>();
  this->interval = std::chrono::duration<double>(
    config::get<float>(name(), "interval", 1));

  this->formatter->add(DEFAULT_FORMAT, TAG_DATE, { TAG_DATE });

  this->date = config::get<std::string>(name(), "date");
  this->date_detailed = config::get<std::string>(name(), "date_detailed", "");

  if (!this->date_detailed.empty())
    register_command_handler(name());
}

bool DateModule::update()
{
  auto date_format = this->detailed ? this->date_detailed : this->date;
  auto time = std::time(nullptr);

  if (this->formatter->has(TAG_DATE))
    std::strftime(this->date_str, sizeof(this->date_str), date_format.c_str(), std::localtime(&time));

  return true;
}

std::string DateModule::get_output()
{
  if (!this->date_detailed.empty())
    this->builder->cmd(Cmd::LEFT_CLICK, EVENT_TOGGLE);

  this->builder->node(this->Module::get_output());

  return this->builder->flush();
}

bool DateModule::build(Builder *builder, const std::string& tag)
{
  if (tag == TAG_DATE)
    builder->node(this->date_str);
  return tag == TAG_DATE;
}

bool DateModule::handle_command(const std::string& cmd)
{
  if (cmd == EVENT_TOGGLE) {
    this->detailed = !this->detailed;
    this->broadcast();
  }

  return cmd == EVENT_TOGGLE;
}
