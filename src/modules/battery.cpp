#include <thread>

#include "config.hpp"
#include "lemonbuddy.hpp"
#include "modules/battery.hpp"
#include "services/logger.hpp"
#include "utils/config.hpp"
#include "utils/io.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"

using namespace modules;

BatteryModule::BatteryModule(const std::string& name_) : InotifyModule(name_)
{
  this->battery = config::get<std::string>(name(), "battery", "BAT0");
  this->adapter = config::get<std::string>(name(), "adapter", "ADP1");
  this->full_at = config::get<int>(name(), "full_at", 100);

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
  if (this->formatter->has(TAG_LABEL_CHARGING, FORMAT_CHARGING))
    this->label_charging = drawtypes::get_optional_config_label(
      name(), get_tag_name(TAG_LABEL_CHARGING), "%percentage%");
  if (this->formatter->has(TAG_LABEL_DISCHARGING, FORMAT_DISCHARGING))
    this->label_discharging = drawtypes::get_optional_config_label(
      name(), get_tag_name(TAG_LABEL_DISCHARGING), "%percentage%");
  if (this->formatter->has(TAG_LABEL_FULL, FORMAT_FULL))
    this->label_full = drawtypes::get_optional_config_label(
      name(), get_tag_name(TAG_LABEL_FULL), "%percentage%");

  this->watch(string::replace(PATH_BATTERY_CAPACITY, "%battery%", this->battery));
  this->watch(string::replace(PATH_ADAPTER_STATUS, "%adapter%", this->adapter));

  if (this->animation_charging) {
    this->threads.emplace_back(std::thread(&BatteryModule::animation_thread_runner, this));
  }
}

void BatteryModule::animation_thread_runner()
{
  while (this->enabled()) {
    // std::unique_lock<std::mutex> lck(this->ev_mtx);

    auto state = this->state();

    if (state & CHARGING && !(state & FULL)) {
      this->broadcast();
    // else
    //   this->cv.wait(lck, [&]{ return this->state & CHARGING && ~this->state & FULL; });
      std::this_thread::sleep_for(std::chrono::duration<double>(
        float(this->animation_charging->get_framerate()) / 1000));
    } else {
      std::this_thread::sleep_for(1s);
    }
  }
}

bool BatteryModule::on_event(InotifyEvent *event)
{
  // std::unique_lock<std::mutex> lck(this->ev_mtx);

  if (event != nullptr)
    log_trace(event->filename);

  auto path_capacity  = string::replace(PATH_BATTERY_CAPACITY, "%battery%", this->battery);
  auto path_status = string::replace(PATH_ADAPTER_STATUS, "%adapter%", this->adapter);
  auto status = io::file::get_contents(path_status);
  int state = UNKNOWN;

  if (status.empty()) {
    log_error("Failed to read "+ path_status);
    return false;
  }

  auto capacity = io::file::get_contents(path_capacity);

  if (capacity.empty()) {
    log_error("Failed to read "+ path_capacity);
    return false;
  }

  this->percentage = (int) math::cap<float>(std::atof(capacity.c_str()), 0, 100) + 0.5;

  switch (status[0]) {
    case '0': state = DISCHARGING; break;
    case '1': state = CHARGING; break;
  }

  if (this->state() & CHARGING && this->percentage >= this->full_at)
    this->percentage = 100;

  if (this->percentage == 100)
    state |= FULL;

  this->state = state;

  if (!this->label_charging_tokenized)
    this->label_charging_tokenized = this->label_charging->clone();
  if (!this->label_discharging_tokenized)
    this->label_discharging_tokenized = this->label_discharging->clone();
  if (!this->label_full_tokenized)
    this->label_full_tokenized = this->label_full->clone();

  auto percentage_str = std::to_string(this->percentage) + "%";

  this->label_charging_tokenized->text = this->label_charging->text;
  this->label_charging_tokenized->replace_token("%percentage%", percentage_str);

  this->label_discharging_tokenized->text = this->label_discharging->text;
  this->label_discharging_tokenized->replace_token("%percentage%", std::to_string(this->percentage) +"%");

  this->label_full_tokenized->text = this->label_full->text;
  this->label_full_tokenized->replace_token("%percentage%", percentage_str);

  // lck.unlock();
  //
  // this->cv.notify_all();

  return true;
}

std::string BatteryModule::get_format()
{
  auto state = this->state();

  if (state & FULL)
    return FORMAT_FULL;
  else if (state & CHARGING)
    return FORMAT_CHARGING;
  else
    return FORMAT_DISCHARGING;
}

bool BatteryModule::build(Builder *builder, const std::string& tag)
{
  if (tag == TAG_ANIMATION_CHARGING)
    builder->node(this->animation_charging);
  else if (tag == TAG_BAR_CAPACITY) {
    builder->node(this->bar_capacity, this->percentage);
    // builder->node(this->bar_capacity, 10);
    // builder->space(5);
    // builder->node(this->bar_capacity, 50);
    // builder->space(5);
    // builder->node(this->bar_capacity, 90);
    // builder->space(5);
    // builder->node(this->bar_capacity, 100);
  } else if (tag == TAG_RAMP_CAPACITY)
    builder->node(this->ramp_capacity, this->percentage);
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
