#pragma once

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
    static constexpr auto TAG_LABEL_TOGGLE = "<label-toggle>";
    static constexpr auto TAG_MENU = "<menu>";

    static constexpr auto EVENT_MENU_OPEN = "menu_open-";
    static constexpr auto EVENT_MENU_CLOSE = "menu_close";

    std::mutex output_mtx;
    std::mutex cmd_mtx;

    int current_level = -1;
    std::vector<std::unique_ptr<MenuTree>> levels;

    std::unique_ptr<drawtypes::Label> label_open;
    std::unique_ptr<drawtypes::Label> label_close;

    public:
      explicit MenuModule(std::string name);

      std::string get_output();

      bool build(Builder *builder, std::string tag);

      bool handle_command(std::string cmd);
  };
}
