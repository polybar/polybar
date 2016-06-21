#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "drawtypes/label.hpp"

namespace drawtypes
{
  struct Icon : public Label
  {
    Icon(std::string icon, int font = 0)
      : Label(icon, font){}
    Icon(std::string icon, std::string fg, std::string bg = "", std::string ul = "", std::string ol = "", int font = 0, int padding = 0, int margin = 0)
      : Label(icon, fg, bg, ul, ol, font, padding, margin){}

    std::unique_ptr<Icon> clone();
  };

  class IconMap
  {
    std::map<std::string, std::unique_ptr<Icon>> icons;

    public:
      void add(std::string id, std::unique_ptr<Icon> &&icon);
      std::unique_ptr<Icon> &get(std::string id, std::string fallback_id = "");
      bool has(std::string id);

      operator bool() {
        return this->icons.size() > 0;
      }
  };

  std::unique_ptr<Icon> get_config_icon(std::string module_name, std::string icon_name = "icon", bool required = true, std::string def = "");
  std::unique_ptr<Icon> get_optional_config_icon(std::string module_name, std::string icon_name = "icon", std::string def = "");
}
