#ifndef _DRAWTYPES_ICON_HPP_
#define _DRAWTYPES_ICON_HPP_

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "drawtypes/label.hpp"

namespace drawtypes
{
  struct Icon : public Label
  {
    Icon(const std::string& icon, int font = 0)
      : Label(icon, font){}
    Icon(const std::string& icon, const std::string& fg, const std::string& bg = "", const std::string& ul = "", const std::string& ol = "", int font = 0, int padding = 0, int margin = 0)
      : Label(icon, fg, bg, ul, ol, font, padding, margin){}

    std::unique_ptr<Icon> clone();
  };

  class IconMap
  {
    std::map<std::string, std::unique_ptr<Icon>> icons;

    public:
      void add(const std::string& id, std::unique_ptr<Icon> &&icon);
      std::unique_ptr<Icon> &get(const std::string& id, const std::string& fallback_id = "");
      bool has(const std::string& id);

      operator bool() {
        return this->icons.size() > 0;
      }
  };

  std::unique_ptr<Icon> get_config_icon(const std::string& module_name, const std::string& icon_name = "icon", bool required = true, const std::string& def = "");
  std::unique_ptr<Icon> get_optional_config_icon(const std::string& module_name, const std::string& icon_name = "icon", const std::string& def = "");
}

#endif
