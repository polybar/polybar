#include "bar.hpp"
#include "config.hpp"
#include "registry.hpp"
#include "services/builder.hpp"
#include "utils/config.hpp"

#include "modules/backlight.hpp"
#include "modules/battery.hpp"
#include "modules/bspwm.hpp"
#include "modules/counter.hpp"
#include "modules/cpu.hpp"
#include "modules/date.hpp"
#include "modules/memory.hpp"
#include "modules/menu.hpp"
#include "modules/script.hpp"
#include "modules/text.hpp"
#if ENABLE_I3
#include "modules/i3.hpp"
#endif
#if ENABLE_MPD
#include "modules/mpd.hpp"
#endif
#if ENABLE_NETWORK
#include "modules/network.hpp"
#endif
#if ENABLE_ALSA
#include "modules/volume.hpp"
#endif

std::shared_ptr<Bar> bar;
std::shared_ptr<Bar> get_bar()
{
  if (bar == nullptr)
    bar = std::make_shared<Bar>();
  return bar;
}

std::shared_ptr<Options> bar_opts() {
  return get_bar()->opts;
}

/**
 * Bar constructor
 */
Bar::Bar() : config_path(config::get_bar_path()), opts(std::make_shared<Options>())
{
  struct Options defaults;

  try {
    this->opts->locale = config::get<std::string>(this->config_path, "locale");
    std::locale::global(std::locale(this->opts->locale.c_str()));
  } catch (config::MissingValueException &e) {}

  auto monitor_name = config::get<std::string>(this->config_path, "monitor", "");
  auto monitors = xcb::monitor::get_list(x::connection(), x::connection().root());

  // In case we're not connected to X, we'll just ignore the monitor
  if (!monitors.empty()) {
    if (monitor_name.empty() && !monitors.empty())
      monitor_name = monitors[0]->name;

    for (auto &&monitor : monitors) {
      if (monitor_name.compare(monitor->name) == 0) {
        this->opts->monitor = monitor;
        break;
      }
    }

    if (!this->opts->monitor)
      throw ConfigurationError("Could not find monitor: "+ monitor_name);
  }

  this->opts->wm_name = "lemonbuddy-"+ this->config_path.substr(4);
  if (this->opts->monitor)
    this->opts->wm_name += "_"+ std::string(this->opts->monitor->name);
  this->opts->wm_name = string::replace(config::get<std::string>(this->config_path, "wm_name", this->opts->wm_name), " ", "-");

  // Create an empty monitor as fallback
  if (!this->opts->monitor)
    this->opts->monitor = xcb::monitor::make_object();

  this->opts->offset_x = config::get<int>(this->config_path, "offset_x", defaults.offset_x);
  this->opts->offset_y = config::get<int>(this->config_path, "offset_y", defaults.offset_y);

  auto width = config::get<std::string>(this->config_path, "width", "100%");
  auto height = config::get<std::string>(this->config_path, "height", "30");

  if (width.find("%") != std::string::npos) {
    this->opts->width = this->opts->monitor->bounds.width * (std::atoi(width.c_str()) / 100.0) + 0.5;
    this->opts->width -= this->opts->offset_x * 2;
  } else {
    this->opts->width = std::atoi(width.c_str());
  }

  if (height.find("%") != std::string::npos) {
    this->opts->height = this->opts->monitor->bounds.height * (std::atoi(height.c_str()) / 100.0) + 0.5;
    this->opts->width -= this->opts->offset_y * 2;
  } else {
    this->opts->height = std::atoi(height.c_str());
  }

  this->opts->background = config::get<std::string>(this->config_path, "background", defaults.background);
  this->opts->foreground = config::get<std::string>(this->config_path, "foreground", defaults.foreground);
  this->opts->linecolor = config::get<std::string>(this->config_path, "linecolor", defaults.linecolor);

  this->opts->bottom = config::get<bool>(this->config_path, "bottom", defaults.bottom);
  this->opts->dock = config::get<bool>(this->config_path, "dock", defaults.dock);
  this->opts->clickareas = config::get<int>(this->config_path, "clickareas", defaults.clickareas);

  this->opts->separator = config::get<std::string>(this->config_path, "separator", defaults.separator);

  this->opts->spacing = config::get<int>(this->config_path, "spacing", defaults.spacing);
  this->opts->lineheight = config::get<int>(this->config_path, "lineheight", defaults.lineheight);

  this->opts->padding_left = config::get<int>(this->config_path, "padding_left", defaults.padding_left);
  this->opts->padding_right = config::get<int>(this->config_path, "padding_right", defaults.padding_right);

  this->opts->module_margin_left = config::get<int>(this->config_path, "module_margin_left", defaults.module_margin_left);
  this->opts->module_margin_right = config::get<int>(this->config_path, "module_margin_right", defaults.module_margin_right);

  for (auto f : config::get_list<std::string>(this->config_path, "font")) {
    std::vector<std::string> font;
    string::split_into(f, ';', font);
    if (font.size() < 2)
      font.emplace_back("0");
    this->opts->fonts.emplace_back(std::make_unique<Font>(font[0], std::stoi(font[1])));
  }
}

/**
 * Loads all modules configured for the current bar
 */
void Bar::load(std::shared_ptr<Registry> registry)
{
  this->registry = registry;

  auto add_modules = [&](std::string modlist, std::vector<std::string> &vec){
    std::vector<std::string> modules;
    string::split_into(modlist, ' ', modules);

    for (auto &&mod : modules) {
      std::unique_ptr<modules::ModuleInterface> module;
      auto type = config::get<std::string>("module/"+ mod, "type");

      if (!ENABLE_ALSA && type == "internal/volume")
        throw CompiledWithoutModuleSupport(type);
      if (!ENABLE_I3 && type == "internal/i3")
        throw CompiledWithoutModuleSupport(type);
      if (!ENABLE_MPD && type == "internal/mpd")
        throw CompiledWithoutModuleSupport(type);
      if (!ENABLE_NETWORK && type == "internal/network")
        throw CompiledWithoutModuleSupport(type);

           if (type == "internal/counter") module = std::make_unique<modules::CounterModule>(mod);
      else if (type == "internal/backlight") module = std::make_unique<modules::BacklightModule>(mod);
      else if (type == "internal/battery") module = std::make_unique<modules::BatteryModule>(mod);
      else if (type == "internal/bspwm") module = std::make_unique<modules::BspwmModule>(mod, this->opts->monitor->name);
      else if (type == "internal/cpu") module = std::make_unique<modules::CpuModule>(mod);
      else if (type == "internal/date") module = std::make_unique<modules::DateModule>(mod);
      else if (type == "internal/memory") module = std::make_unique<modules::MemoryModule>(mod);
      #if ENABLE_I3
      else if (type == "internal/i3") module = std::make_unique<modules::i3Module>(mod, this->opts->monitor->name);
      #endif
      #if ENABLE_MPD
      else if (type == "internal/mpd") module = std::make_unique<modules::MpdModule>(mod);
      #endif
      #if ENABLE_ALSA
      else if (type == "internal/volume") module = std::make_unique<modules::VolumeModule>(mod);
      #endif
      #if ENABLE_NETWORK
      else if (type == "internal/network") module = std::make_unique<modules::NetworkModule>(mod);
      #endif
      else if (type == "custom/text") module = std::make_unique<modules::TextModule>(mod);
      else if (type == "custom/script") module = std::make_unique<modules::ScriptModule>(mod);
      else if (type == "custom/menu") module = std::make_unique<modules::MenuModule>(mod);
      else throw ConfigurationError("Unknown module: "+ mod);

      module->attach_registry(this->registry);

      vec.emplace_back(module->name());
      this->registry->insert(std::move(module));
    }
  };

  add_modules(config::get<std::string>(this->config_path, "modules-left", ""), this->mod_left);
  add_modules(config::get<std::string>(this->config_path, "modules-center", ""), this->mod_center);
  add_modules(config::get<std::string>(this->config_path, "modules-right", ""), this->mod_right);

  if (this->mod_left.empty() && this->mod_center.empty() && this->mod_right.empty())
    throw ConfigurationError("The bar does not contain any modules...");
}

/**
 * Builds the output string by combining the output
 * of all added modules
 */
std::string Bar::get_output()
{
  auto builder = std::make_unique<Builder>();
  auto pad_left = std::string(this->opts->padding_left, ' ');
  auto pad_right = std::string(this->opts->padding_right, ' ');

  auto output_modules = [&](Builder::Alignment align, std::vector<std::string> &mods){
    if (!pad_left.empty() && !mods.empty() && align == Builder::ALIGN_LEFT)
      builder->append_module_output(align, pad_left, false, false);

    for (auto &mod_name : mods) {
      auto mod_output = this->registry->get(mod_name);
      builder->append_module_output(align, mod_output,
          !(align == Builder::ALIGN_LEFT && mod_name == mods.front()),
          !(align == Builder::ALIGN_RIGHT && mod_name == mods.back()));

      if (!mod_output.empty() && !this->opts->separator.empty() && mod_name != mods.back())
        builder->append(this->opts->separator);
    }

    if (!pad_right.empty() && !mods.empty() && align == Builder::ALIGN_RIGHT)
      builder->append_module_output(align, pad_right, false, false);
  };

  output_modules(Builder::ALIGN_LEFT, this->mod_left);
  output_modules(Builder::ALIGN_CENTER, this->mod_center);
  output_modules(Builder::ALIGN_RIGHT, this->mod_right);

  auto data = builder->flush();

  // return data;
  return string::replace_all(data, "}%{", " ");
}


/**
 * Builds the command line string used to execute
 * lemonbar using the specified bar
 */
std::string Bar::get_exec_line()
{
  std::stringstream buffer;

  buffer << "lemonbar -p";
  if (!this->opts->wm_name.empty())
    buffer << " -n " << this->opts->wm_name;
  if (!this->opts->background.empty())
    buffer << " -B " << this->opts->background;
  if (!this->opts->foreground.empty())
    buffer << " -F " << this->opts->foreground;
  if (!this->opts->linecolor.empty())
    buffer << " -U " << this->opts->linecolor;
  if (this->opts->bottom)
    buffer << " -b ";
  if (this->opts->dock)
    buffer << " -d ";

  for (auto &&font : this->opts->fonts) {
    buffer << " -f " << string::replace(font->id, " ", "-");
    buffer << " -o " << font->offset;
  }

  buffer << " -u " << this->opts->lineheight;
  buffer << " -a " << this->opts->clickareas;
  buffer << " -g " << this->opts->get_geom();
  buffer << " " << this->opts->monitor->name;

  buffer.flush();

  return string::squeeze(buffer.str(), ' ');
}
