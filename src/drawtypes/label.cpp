#include "drawtypes/label.hpp"
#include "utils/config.hpp"
#include "utils/string.hpp"

namespace drawtypes
{
  std::unique_ptr<Label> Label::clone() {
    return std::unique_ptr<Label> { new Label(
      this->text, this->fg, this->bg, this->ul, this->ol, this->font, this->padding, this->margin, this->maxlen, this->ellipsis) };
  }

  void Label::replace_token(std::string token, std::string replacement) {
    this->text = string::replace_all(this->text, token, replacement);
  }

  void Label::replace_defined_values(std::unique_ptr<Label> &label)
  {
    if (!label->fg.empty()) this->fg = label->fg;
    if (!label->bg.empty()) this->bg = label->bg;
    if (!label->ul.empty()) this->ul = label->ul;
    if (!label->ol.empty()) this->ol = label->ol;
  }

  std::unique_ptr<Label> get_config_label(std::string config_path, std::string label_name, bool required, std::string def)
  {
    std::string label;

    if (required)
      label = config::get<std::string>(config_path, label_name);
    else
      label = config::get<std::string>(config_path, label_name, def);

    return std::unique_ptr<Label> { new Label(label,
      config::get<std::string>(config_path, label_name +"-foreground", ""),
      config::get<std::string>(config_path, label_name +"-background", ""),
      config::get<std::string>(config_path, label_name +"-underline", ""),
      config::get<std::string>(config_path, label_name +"-overline", ""),
      config::get<int>(config_path, label_name +"-font", 0),
      config::get<int>(config_path, label_name +"-padding", 0),
      config::get<int>(config_path, label_name +"-margin", 0),
      config::get<size_t>(config_path, label_name +"-maxlen", 0),
      config::get<bool>(config_path, label_name +"-ellipsis", true)) };
  }

  std::unique_ptr<Label> get_optional_config_label(std::string config_path, std::string label_name, std::string def) {
    return get_config_label(config_path, label_name, false, def);
  }
}
