#include "lemonbuddy.hpp"
#include "registry.hpp"
#include "modules/base.hpp"
#include "utils/config.hpp"
#include "utils/string.hpp"

std::string ModuleFormatter::Format::decorate(Builder *builder, std::string output)
{
  if (this->offset != 0) builder->offset(this->offset);

  if (this->margin > 0) builder->space(this->margin);

  if (!this->bg.empty()) builder->background(this->bg);
  if (!this->fg.empty()) builder->color(this->fg);
  if (!this->ul.empty()) builder->underline(this->ul);
  if (!this->ol.empty()) builder->overline(this->ol);

  if (this->padding > 0) builder->space(this->padding);

  builder->append(output);

  if (this->padding > 0) builder->space(this->padding);

  if (!this->ol.empty()) builder->overline_close();
  if (!this->ul.empty()) builder->underline_close();
  if (!this->fg.empty()) builder->color_close();
  if (!this->bg.empty()) builder->background_close();

  if (this->margin > 0) builder->space(this->margin);

  return builder->flush();
}

void ModuleFormatter::add(std::string name, std::string fallback, std::vector<std::string>&& tags, std::vector<std::string>&& whitelist)
{
  auto format = std::make_unique<Format>();

  format->value = config::get<std::string>(this->module_name, name, fallback);
  format->fg = config::get<std::string>(this->module_name, name +"-foreground", "");
  format->bg = config::get<std::string>(this->module_name, name +"-background", "");
  format->ul = config::get<std::string>(this->module_name, name +"-underline", "");
  format->ol = config::get<std::string>(this->module_name, name +"-overline", "");
  format->spacing = config::get<int>(this->module_name, name +"-spacing", DEFAULT_SPACING);
  format->padding = config::get<int>(this->module_name, name +"-padding", 0);
  format->margin = config::get<int>(this->module_name, name +"-margin", 0);
  format->offset = config::get<int>(this->module_name, name +"-offset", 0);
  format->tags.swap(tags);

  for (auto &&tag : string::split(format->value, ' ')) {
    if (tag[0] != '<' || tag[tag.length()-1] != '>')
      continue;
    if (std::find(format->tags.begin(), format->tags.end(), tag) != format->tags.end())
      continue;
    if (std::find(whitelist.begin(), whitelist.end(), tag) != whitelist.end())
      continue;
    throw UndefinedFormatTag("["+ this->module_name +"] Undefined \""+ name +"\" tag: "+ tag);
  }

  this->formats.insert(std::make_pair(name, std::move(format)));
}

std::unique_ptr<ModuleFormatter::Format>& ModuleFormatter::get(std::string format_name)
{
  auto format = this->formats.find(format_name);
  if (format == this->formats.end())
    throw UndefinedFormat("Format \""+ format_name +"\" has not been added");
  return format->second;
}

bool ModuleFormatter::has(std::string tag, std::string format_name)
{
  auto format = this->formats.find(format_name);
  if (format == this->formats.end())
    throw UndefinedFormat(format_name);
  return format->second->value.find(tag) != std::string::npos;
}

bool ModuleFormatter::has(std::string tag)
{
  for (auto &&format : this->formats)
    if (format.second->value.find(tag) != std::string::npos)
      return true;
  return false;
}

namespace modules
{
  void broadcast_module_update(std::string module_name)
  {
    log_trace("Broadcasting module update for => "+ module_name);
    get_registry()->notify(module_name);
  }

  std::string get_tag_name(std::string tag) {
    return tag.length() < 2 ? "" : tag.substr(1, tag.length()-2);
  }
}
