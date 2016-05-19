#ifndef _DRAWTYPES_LABEL_HPP_
#define _DRAWTYPES_LABEL_HPP_

#include <string>
#include <memory>

namespace drawtypes
{
  struct Label
  {
    std::string text, fg, bg, ul, ol;
    int font = 0, padding = 0, margin = 0;

    Label(const std::string& text, int font)
      : text(text), font(font){}
    Label(const std::string& text, const std::string& fg = "", const std::string& bg = "", const std::string& ul = "", const std::string& ol = "", int font = 0, int padding = 0, int margin = 0)
      : text(text), fg(fg), bg(bg), ul(ul), ol(ol), font(font), padding(padding), margin(margin){}

    operator bool() {
      return !this->text.empty();
    }

    std::unique_ptr<Label> clone();

    void replace_token(const std::string& token, const std::string& replacement);
    void replace_defined_values(std::unique_ptr<Label> &label);
  };

  std::unique_ptr<Label> get_config_label(const std::string& module_name, const std::string& label_name = "label", bool required = true, const std::string& def = "");
  std::unique_ptr<Label> get_optional_config_label(const std::string& module_name, const std::string& label_name = "label", const std::string& def = "");
}

#endif
