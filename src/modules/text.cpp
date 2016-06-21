#include "lemonbuddy.hpp"
#include "bar.hpp"
#include "modules/text.hpp"
#include "utils/config.hpp"

using namespace modules;

TextModule::TextModule(std::string name_) : StaticModule(name_) {
  this->formatter->add(FORMAT, "", {});
  if (this->formatter->get(FORMAT)->value.empty())
    throw UndefinedFormat(FORMAT);
}

std::string TextModule::get_format() {
  return FORMAT;
}

std::string TextModule::get_output()
{
  auto click_left = config::get<std::string>(name(), "click-left", "");
  auto click_middle = config::get<std::string>(name(), "click-middle", "");
  auto click_right = config::get<std::string>(name(), "click-right", "");

  auto scroll_up = config::get<std::string>(name(), "scroll-up", "");
  auto scroll_down = config::get<std::string>(name(), "scroll-down", "");

  if (!click_left.empty())
    this->builder->cmd(Cmd::LEFT_CLICK, click_left);
  if (!click_middle.empty())
    this->builder->cmd(Cmd::MIDDLE_CLICK, click_middle);
  if (!click_right.empty())
    this->builder->cmd(Cmd::RIGHT_CLICK, click_right);

  if (!scroll_up.empty())
    this->builder->cmd(Cmd::SCROLL_UP, scroll_up);
  if (!scroll_down.empty())
    this->builder->cmd(Cmd::SCROLL_DOWN, scroll_down);

  this->builder->node(this->Module::get_output());

  return this->builder->flush();
}
