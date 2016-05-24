#ifndef _BAR_HPP_
#define _BAR_HPP_

#include <string>
#include <memory>

#include "config.hpp"
#include "exception.hpp"
#include "utils/xlib.hpp"

DefineBaseException(ConfigurationError);

struct CompiledWithoutModuleSupport : public ConfigurationError
{
  CompiledWithoutModuleSupport(std::string module_name)
    : ConfigurationError(std::string(APP_NAME) + " was not compiled with support for module \""+ module_name +"\"") {}
};

struct Font
{
  std::string id;
  int offset;

  Font(const std::string& id, int offset)
    : id(id), offset(offset){}
};

enum Cmd
{
  LEFT_CLICK = 1,
  MIDDLE_CLICK = 2,
  RIGHT_CLICK = 3,
  SCROLL_UP = 4,
  SCROLL_DOWN = 5,
};

struct Options
{
  std::unique_ptr<xlib::Monitor> monitor;
  std::string wm_name;
  std::string locale;

  std::string background = "#ffffff";
  std::string foreground = "#000000";
  std::string linecolor = "#000000";

  int width;
  int height;

  int offset_x = 0;
  int offset_y = 0;

  bool bottom = false;
  bool dock = true;
  int clickareas = 25;

  std::string separator;
  int spacing = 1;
  int lineheight = 1;

  int padding_left = 0;
  int padding_right = 0;
  int module_margin_left = 0;
  int module_margin_right = 2;

  std::vector<std::unique_ptr<Font>> fonts;

  std::string get_geom()
  {
    std::stringstream ss;
    ss << this->width << "x" << this->height << "+";
    ss << this->offset_x << "+" << this->offset_y;
    return ss.str();
  }
};

class Bar
{
  std::string config_path;

  std::vector<std::string> mod_left;
  std::vector<std::string> mod_center;
  std::vector<std::string> mod_right;

  public:
    Bar();

    std::unique_ptr<Options> opts;

    std::string get_output();
    std::string get_exec_line();

    void load();
};

std::shared_ptr<Bar> &get_bar();

const Options& bar_opts();

#endif
