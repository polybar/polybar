#include "modules/backlight.hpp"
#include "utils/config.hpp"
#include "utils/io.hpp"

using namespace modules;

BacklightModule::BacklightModule(const std::string& name_) : InotifyModule(name_)
{
  this->formatter->add(DEFAULT_FORMAT, TAG_LABEL, { TAG_LABEL, TAG_BAR, TAG_RAMP });

  if (this->formatter->has(TAG_LABEL))
    this->label = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL), "%percentage%");
  if (this->formatter->has(TAG_BAR))
    this->bar = drawtypes::get_config_bar(name(), get_tag_name(TAG_BAR));
  if (this->formatter->has(TAG_RAMP))
    this->ramp = drawtypes::get_config_ramp(name(), get_tag_name(TAG_RAMP));

  if (this->label)
    this->label_tokenized = this->label->clone();

  auto card = config::get<std::string>(name(), "card");

  this->path_val = string::replace(PATH_BACKLIGHT_VAL, "%card%", card);
  this->path_max = string::replace(PATH_BACKLIGHT_MAX, "%card%", card);

  if (!io::file::exists(this->path_val))
    throw ModuleError("[BacklightModule] The file \""+ this->path_val +"\" does not exist");
  if (!io::file::exists(this->path_max))
    throw ModuleError("[BacklightModule] The file \""+ this->path_max +"\" does not exist");

  this->watch(this->path_val);
}

bool BacklightModule::on_event(InotifyEvent *event)
{
  if (event != nullptr)
    log_trace(event->filename);

  auto val = io::file::get_contents(this->path_val);
  this->val = std::stoull(val.c_str(), 0, 10);

  auto max = io::file::get_contents(this->path_max);
  this->max = std::stoull(max.c_str(), 0, 10);

  this->percentage = (int) float(this->val) / float(this->max) * 100.f + 0.5f;

  if (!this->label)
    return true;

  this->label_tokenized->text = this->label->text;
  this->label_tokenized->replace_token("%percentage%", std::to_string(this->percentage)+"%");

  return true;
}

bool BacklightModule::build(Builder *builder, const std::string& tag)
{
  if (tag == TAG_BAR)
    builder->node(this->bar, this->percentage);
  else if (tag == TAG_RAMP)
    builder->node(this->ramp, this->percentage);
  else if (tag == TAG_LABEL)
    builder->node(this->label_tokenized);
  else
    return false;

  return true;
}
