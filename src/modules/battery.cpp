#include <thread>

#include "config.hpp"
#include "lemonbuddy.hpp"
#include "modules/battery.hpp"
#include "services/logger.hpp"
#include "utils/config.hpp"
#include "utils/io.hpp"
#include "utils/math.hpp"
#include "utils/macros.hpp"
#include "utils/string.hpp"

using namespace modules;

const int modules::BatteryModule::STATE_UNKNOWN;
const int modules::BatteryModule::STATE_CHARGING;
const int modules::BatteryModule::STATE_DISCHARGING;
const int modules::BatteryModule::STATE_FULL;

BatteryModule::BatteryModule(std::string name_) : InotifyModule(name_)
{
  this->battery = config::get<std::string>(name(), "battery", "BAT0");
  this->adapter = config::get<std::string>(name(), "adapter", "ADP1");
  this->full_at = config::get<int>(name(), "full_at", 100);

  this->state = STATE_UNKNOWN;
  this->percentage = 0;

  this->formatter->add(FORMAT_CHARGING, TAG_LABEL_CHARGING,
    { TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_ANIMATION_CHARGING, TAG_LABEL_CHARGING });

  this->formatter->add(FORMAT_DISCHARGING, TAG_LABEL_DISCHARGING,
    { TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_LABEL_DISCHARGING });

  this->formatter->add(FORMAT_FULL, TAG_LABEL_FULL,
    { TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_LABEL_FULL });

  if (this->formatter->has(TAG_ANIMATION_CHARGING, FORMAT_CHARGING))
    this->animation_charging = drawtypes::get_config_animation(
      name(), get_tag_name(TAG_ANIMATION_CHARGING));
  if (this->formatter->has(TAG_BAR_CAPACITY))
    this->bar_capacity = drawtypes::get_config_bar(
      name(), get_tag_name(TAG_BAR_CAPACITY));
  if (this->formatter->has(TAG_RAMP_CAPACITY))
    this->ramp_capacity = drawtypes::get_config_ramp(
      name(), get_tag_name(TAG_RAMP_CAPACITY));
  if (this->formatter->has(TAG_LABEL_CHARGING, FORMAT_CHARGING)) {
    this->label_charging = drawtypes::get_optional_config_label(
        name(), get_tag_name(TAG_LABEL_CHARGING), "%percentage%");
    this->label_charging_tokenized = this->label_charging->clone();
  }
  if (this->formatter->has(TAG_LABEL_DISCHARGING, FORMAT_DISCHARGING)) {
    this->label_discharging = drawtypes::get_optional_config_label(
        name(), get_tag_name(TAG_LABEL_DISCHARGING), "%percentage%");
    this->label_discharging_tokenized = this->label_discharging->clone();
  }
  if (this->formatter->has(TAG_LABEL_FULL, FORMAT_FULL)) {
    this->label_full = drawtypes::get_optional_config_label(
        name(), get_tag_name(TAG_LABEL_FULL), "%percentage%");
    this->label_full_tokenized = this->label_full->clone();
  }

  this->path_capacity = string::replace(PATH_BATTERY_CAPACITY, "%battery%", this->battery);
  this->path_adapter = string::replace(PATH_ADAPTER_STATUS, "%adapter%", this->adapter);

  if (!io::file::exists(this->path_capacity))
    throw ModuleError("[BatteryModule] The file \""+ this->path_capacity +"\" does not exist");
  if (!io::file::exists(this->path_adapter))
    throw ModuleError("[BatteryModule] The file \""+ this->path_adapter +"\" does not exist");

  this->watch(this->path_capacity, InotifyEvent::ACCESSED);
  this->watch(this->path_adapter, InotifyEvent::ACCESSED);
}

void BatteryModule::start()
{
  this->InotifyModule::start();
  this->threads.emplace_back(std::thread(&BatteryModule::subthread_routine, this));
}

void BatteryModule::subthread_routine()
{
  std::this_thread::yield();

  std::chrono::duration<double> dur = 1s;

  if (this->animation_charging)
    dur = std::chrono::duration<double>(
      float(this->animation_charging->get_framerate()) / 1000.0f);

  int i = 0;
  const int poll_seconds = config::get<float>(name(), "poll_interval", 3.0f) / dur.count();

  while (this->enabled()) {
    // TODO(jaagr): Keep track of when the values were last read to determine
    // if we need to trigger the event manually or not.
    if (poll_seconds > 0 && (++i % poll_seconds) == 0) {
      // Trigger an inotify event in case the underlying filesystem doesn't
      log_debug("Poll battery capacity");
      io::file::get_contents(this->path_capacity);
      i = 0;
    }

    if (this->state == STATE_CHARGING)
      this->broadcast();

    this->sleep(dur);
  }

  log_trace("Reached end of battery subthread");
}

bool BatteryModule::on_event(InotifyEvent *event)
{
  if (event != nullptr)
    log_trace(event->filename);

  int state = STATE_UNKNOWN;
  auto status = io::file::get_contents(this->path_adapter);

  if (status.empty()) {
    log_error("Failed to read "+ this->path_adapter);
    return false;
  }

  auto capacity = io::file::get_contents(this->path_capacity);

  if (capacity.empty()) {
    log_error("Failed to read "+ this->path_capacity);
    return false;
  }

  int percentage = (int) math::cap<float>(std::atof(capacity.c_str()), 0, 100) + 0.5;

  switch (status[0]) {
    case '0': state = STATE_DISCHARGING; break;
    case '1': state = STATE_CHARGING; break;
  }

  if (state == STATE_CHARGING) {
    if (percentage >= this->full_at)
      percentage = 100;

    if (percentage == 100)
      state = STATE_FULL;
  }

  if (this->state == state && this->percentage == percentage)
    return false;

  this->label_charging_tokenized->text = this->label_charging->text;
  this->label_charging_tokenized->replace_token("%percentage%", IntToStr(percentage) + "%");

  this->label_discharging_tokenized->text = this->label_discharging->text;
  this->label_discharging_tokenized->replace_token("%percentage%", IntToStr(percentage) + "%");

  this->label_full_tokenized->text = this->label_full->text;
  this->label_full_tokenized->replace_token("%percentage%", IntToStr(percentage) + "%");

  this->state = state;
  this->percentage = percentage;

  return true;
}

std::string BatteryModule::get_format()
{
  int state = this->state();

  if (state == STATE_FULL)
    return FORMAT_FULL;
  else if (state == STATE_CHARGING)
    return FORMAT_CHARGING;
  else
    return FORMAT_DISCHARGING;
}

bool BatteryModule::build(Builder *builder, std::string tag)
{
  if (tag == TAG_ANIMATION_CHARGING)
    builder->node(this->animation_charging);
  else if (tag == TAG_BAR_CAPACITY) {
    builder->node(this->bar_capacity, this->percentage());
  } else if (tag == TAG_RAMP_CAPACITY)
    builder->node(this->ramp_capacity, this->percentage());
  else if (tag == TAG_LABEL_CHARGING)
    builder->node(this->label_charging_tokenized);
  else if (tag == TAG_LABEL_DISCHARGING)
    builder->node(this->label_discharging_tokenized);
  else if (tag == TAG_LABEL_FULL)
    builder->node(this->label_full_tokenized);
  else
    return false;

  return true;
}
