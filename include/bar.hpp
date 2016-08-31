#pragma once

#include <string>
#include <memory>
#include <vector>
#include <sstream>

#include "config.hpp"
#include "exception.hpp"
#include "utils/xcb.hpp"

DefineBaseException(ConfigurationError);

class Registry;

struct CompiledWithoutModuleSupport : public ConfigurationError
{
  explicit CompiledWithoutModuleSupport(std::string module_name)
    : ConfigurationError(std::string(APP_NAME) + " was not compiled with support for module \""+ module_name +"\"") {}
};

struct Font
{
  std::string id;
  int offset = 0;

  Font(std::string id, int offset)
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
  std::shared_ptr<xcb::monitor_t> monitor;

  std::string wm_name;
  std::string locale;

  std::string background = "#ffffff";
  std::string foreground = "#000000";
  std::string linecolor = "#000000";

  int width = 0;
  int height = 0;

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
    ss.imbue(std::locale::classic());
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

    std::shared_ptr<Options> opts;
    std::shared_ptr<Registry> registry;

    void load(std::shared_ptr<Registry> registry);

    std::string get_output();
    std::string get_exec_line();
};

std::shared_ptr<Bar> get_bar();
std::shared_ptr<Options> bar_opts();
