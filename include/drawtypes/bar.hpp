#ifndef _DRAWTYPES_BAR_HPP_
#define _DRAWTYPES_BAR_HPP_

#include <string>
#include <memory>
#include <vector>

#include "drawtypes/icon.hpp"

class Builder;

namespace drawtypes
{
  class Bar
  {
    protected:
      std::unique_ptr<Builder> builder;
      std::vector<std::string> colors;
      bool gradient;
      unsigned int width;
      std::string format;

      std::unique_ptr<Icon> fill;
      std::unique_ptr<Icon> empty;
      std::unique_ptr<Icon> indicator;

    public:
      Bar(int width, const std::string& format, bool lazy_builder_closing = true);
      Bar(int width, bool lazy_builder_closing = true)
        : Bar(width, "<fill><indicator><empty>", lazy_builder_closing){}

      void set_fill(std::unique_ptr<Icon> &&icon);
      void set_empty(std::unique_ptr<Icon> &&icon);
      void set_indicator(std::unique_ptr<Icon> &&icon);

      void set_gradient(bool mode);
      void set_colors(std::vector<std::string> &&colors);

      std::string get_output(float percentage, bool floor_percentage = false);
  };

  std::unique_ptr<Bar> get_config_bar(const std::string& config_path, const std::string& bar_name = "bar", bool lazy_builder_closing = true);
}

#endif
