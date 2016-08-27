#pragma once

#include <string>
#include <memory>

namespace drawtypes
{
  struct Label
  {
    std::string text, fg, bg, ul, ol;
    int font = 0, padding = 0, margin = 0;

    Label(std::string text, int font)
      : text(text), font(font){}
    Label(std::string text, std::string fg = "", std::string bg = "", std::string ul = "", std::string ol = "", int font = 0, int padding = 0, int margin = 0)
      : text(text), fg(fg), bg(bg), ul(ul), ol(ol), font(font), padding(padding), margin(margin){}

    operator bool() {
      return !this->text.empty();
    }

    std::unique_ptr<Label> clone();

    void replace_token(std::string token, std::string replacement);
    void replace_defined_values(std::unique_ptr<Label> &label);
  };

  std::unique_ptr<Label> get_config_label(std::string module_name, std::string label_name = "label", bool required = true, std::string def = "");
  std::unique_ptr<Label> get_optional_config_label(std::string module_name, std::string label_name = "label", std::string def = "");
  std::unique_ptr<Label> get_either_config_label(std::string config_path, std::string label_name1, std::string label_name2, std::string def);
}
