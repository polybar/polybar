#include "bar.hpp"
#include "lemonbuddy.hpp"
#include "modules/menu.hpp"
#include "utils/config.hpp"

using namespace modules;

MenuModule::MenuModule(const std::string& name_) : StaticModule(name_)
{
  auto default_format_string = std::string(TAG_LABEL_TOGGLE) +" "+ std::string(TAG_MENU);

  this->formatter->add(DEFAULT_FORMAT, default_format_string, { TAG_LABEL_TOGGLE, TAG_MENU });

  if (this->formatter->has(TAG_LABEL_TOGGLE)) {
    this->label_open = drawtypes::get_config_label(name(), "label:open");
    this->label_close = drawtypes::get_optional_config_label(name(), "label:close", "x");
  }

  if (this->formatter->has(TAG_MENU)) {
    int level_n = 0;

    while (true) {
      auto level_path = "menu:"+ std::to_string(level_n);

      if (config::get<std::string>(name(), level_path +":0", "") == "")
        break;

      this->levels.emplace_back(std::make_unique<MenuTree>());

      int item_n = 0;

      while (true) {
        auto item_path = level_path +":"+ std::to_string(item_n);

        if (config::get<std::string>(name(), item_path, "") == "")
          break;

        auto item = std::make_unique<MenuTreeItem>();

        item->label = drawtypes::get_config_label(name(), item_path);
        item->exec = config::get<std::string>(name(), item_path +":exec", EVENT_MENU_CLOSE);

        this->levels.back()->items.emplace_back(std::move(item));

        item_n++;
      }

      level_n++;
    }
  }

  register_command_handler(name());
}

std::string MenuModule::get_output() throw(UndefinedFormat)
{
  this->builder->node(this->Module::get_output());

  return this->builder->flush();
}

bool MenuModule::build(Builder *builder, const std::string& tag)
{
  if (tag == TAG_LABEL_TOGGLE && this->current_level == -1) {
    builder->cmd(Cmd::LEFT_CLICK, std::string(EVENT_MENU_OPEN) +"0");
      builder->node(this->label_open);
    builder->cmd_close(true);

  } else if (tag == TAG_LABEL_TOGGLE && this->current_level > -1) {
    builder->cmd(Cmd::LEFT_CLICK, EVENT_MENU_CLOSE);
      builder->node(this->label_close);
    builder->cmd_close(true);

  } else if (tag == TAG_MENU && this->current_level > -1) {
    int i = 0;

    for (auto &&m : this->levels[this->current_level]->items) {
      if (i++ > 0)
        builder->space();

      builder->color_alpha("77");
        builder->node("/");
      builder->color_close(true);
      builder->space();

      builder->cmd(Cmd::LEFT_CLICK, m->exec);
        builder->node(m->label);
      builder->cmd_close(true);
    }
  } else {
    return false;
  }

  return true;
}

bool MenuModule::handle_command(const std::string& cmd)
{
  std::lock_guard<std::mutex> lck(this->cmd_mtx);

  if (cmd.find(EVENT_MENU_OPEN) == 0) {
    auto level = cmd.substr(std::strlen(EVENT_MENU_OPEN));

    if (level.empty())
      level = "0";

    this->current_level = std::atoi(level.c_str());

    if (this->current_level >= (int) this->levels.size()) {
      log_error("Cannot open unexisting menu level: "+ level);
      this->current_level = -1;
    }

  } else if (cmd == EVENT_MENU_CLOSE) {
    this->current_level = -1;
  } else {
    this->current_level = -1;
    this->broadcast();
    return false;
  }

  this->broadcast();

  return true;
}
