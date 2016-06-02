#ifndef _MODULES_MENU_HPP_
#define _MODULES_MENU_HPP_

#include <mutex>

#include "modules/base.hpp"

namespace modules
{
  struct MenuTreeItem {
    std::string exec;
    std::unique_ptr<drawtypes::Label> label;
  };

  struct MenuTree {
    std::vector<std::unique_ptr<MenuTreeItem>> items;
  };

  DefineModule(MenuModule, StaticModule)
  {
    const char *TAG_LABEL_TOGGLE = "<label-toggle>";
    const char *TAG_MENU = "<menu>";

    const char *EVENT_MENU_OPEN = "menu_open-";
    const char *EVENT_MENU_CLOSE = "menu_close";

    std::mutex output_mtx;
    std::mutex cmd_mtx;

    int current_level = -1;
    std::vector<std::unique_ptr<MenuTree>> levels;

    std::unique_ptr<drawtypes::Label> label_open;
    std::unique_ptr<drawtypes::Label> label_close;

    public:
      MenuModule(const std::string& name);

      std::string get_output() throw(UndefinedFormat);

      bool build(Builder *builder, const std::string& tag);

      bool handle_command(const std::string& cmd);
  };
}

#endif
