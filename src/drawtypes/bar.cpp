#include "services/builder.hpp"
#include "lemonbuddy.hpp"
#include "drawtypes/bar.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"
#include "utils/config.hpp"

namespace drawtypes
{
  std::unique_ptr<Bar> get_config_bar(std::string config_path, std::string bar_name, bool lazy_builder_closing)
  {
    std::unique_ptr<Bar> bar;

    auto width = config::get<int>(config_path, bar_name +"-width");
    auto format = config::get<std::string>(config_path, bar_name +"-format", "%fill%%indicator%%empty%");

    if (format.empty())
      bar = std::make_unique<Bar>(width, lazy_builder_closing);
    else
      bar = std::make_unique<Bar>(width, format, lazy_builder_closing);

    bar->set_gradient(config::get<bool>(config_path, bar_name +"-gradient", true));
    bar->set_colors(config::get_list<std::string>(config_path, bar_name +"-foreground", {}));
    bar->set_indicator(get_config_icon(config_path, bar_name +"-indicator", format.find("%indicator%") != std::string::npos, ""));
    bar->set_fill(get_config_icon(config_path, bar_name +"-fill", format.find("%fill%") != std::string::npos, ""));
    bar->set_empty(get_config_icon(config_path, bar_name +"-empty", format.find("%empty%") != std::string::npos, ""));

    return bar;
  }

  Bar::Bar(int width, std::string fmt, bool lazy_builder_closing)
    : builder(std::make_unique<Builder>(lazy_builder_closing)), format(fmt)
  {
    this->width = width;
  }

  void Bar::set_fill(std::unique_ptr<Icon> &&fill) {
    this->fill.swap(fill);
  }

  void Bar::set_empty(std::unique_ptr<Icon> &&empty) {
    this->empty.swap(empty);
  }

  void Bar::set_indicator(std::unique_ptr<Icon> &&indicator) {
    this->indicator.swap(indicator);
  }

  void Bar::set_gradient(bool mode) {
    this->gradient = mode;
  }

  void Bar::set_colors(std::vector<std::string> &&colors) {
    this->colors.swap(colors);
  }

  std::string Bar::get_output(float percentage, bool floor_percentage)
  {
    if (this->colors.empty()) this->colors.emplace_back(this->fill->fg);

    float add = floor_percentage ? 0 : 0.5;
    int fill_width = (int) this->width * percentage / 100 + add;
    int empty_width = this->width - fill_width;
    int color_step = this->width / this->colors.size() + 0.5f;

    auto output = std::string(this->format);

    if (*this->indicator) {
      if (empty_width == 1)
        empty_width = 0;
      else if (fill_width == 0)
        empty_width--;
      else
        fill_width--;
    }

    if (!this->gradient) {
      auto color = this->colors.size() * percentage / 100;
      if (color >= this->colors.size())
        color = 0;
      this->fill->fg = this->colors[color-1];
      while (fill_width--) this->builder->node(this->fill);
    } else {
      int i = 0;
      for (auto color : this->colors) {
        i += 1;
        int j = 0;

        if ((i-1) * color_step >= fill_width) break;

        this->fill->fg = color;

        while (j++ < color_step && (i-1) * color_step + j <= fill_width)
          this->builder->node(this->fill);
      }
    }

    auto fill = this->builder->flush();

    this->builder->node(this->indicator);
    auto indicator = this->builder->flush();

    while (empty_width--) this->builder->node(this->empty);
    auto empty = this->builder->flush();

    output = string::replace_all(output, "%fill%", fill);
    output = string::replace_all(output, "%indicator%", indicator);
    output = string::replace_all(output, "%empty%", empty);

    return output;
  }
}
