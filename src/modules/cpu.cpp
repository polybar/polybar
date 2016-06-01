#include "config.hpp"
#include "modules/cpu.hpp"
#include "utils/config.hpp"
#include "utils/math.hpp"
#include "utils/memory.hpp"
#include "utils/string.hpp"

using namespace modules;

CpuModule::CpuModule(const std::string& name_) : TimerModule(name_, 1s)
{
  this->interval = std::chrono::duration<double>(
    config::get<float>(name(), "interval", 1));

  this->formatter->add(DEFAULT_FORMAT, TAG_LABEL, {
    TAG_LABEL, TAG_BAR_LOAD, TAG_RAMP_LOAD, TAG_RAMP_LOAD_PER_CORE });

  if (this->formatter->has(TAG_LABEL))
    this->label = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL), "%percentage%");
  if (this->formatter->has(TAG_BAR_LOAD))
    this->bar_load = drawtypes::get_config_bar(name(), get_tag_name(TAG_BAR_LOAD));
  if (this->formatter->has(TAG_RAMP_LOAD))
    this->ramp_load = drawtypes::get_config_ramp(name(), get_tag_name(TAG_RAMP_LOAD));
  if (this->formatter->has(TAG_RAMP_LOAD_PER_CORE))
    this->ramp_load_per_core = drawtypes::get_config_ramp(name(), get_tag_name(TAG_RAMP_LOAD_PER_CORE));

  if (this->label)
    this->label_tokenized = this->label->clone();

  // warmup cpu_time and prev_cpu_time
  this->read_values();
  this->read_values();
}

bool CpuModule::update()
{
  if (!this->read_values())
    return false;

  this->current_total_load = 0.0f;
  this->current_load.clear();

  int cores_n = this->cpu_times.size();

  if (cores_n == 0)
    return false;

  repeat(cores_n)
  {
    auto load = this->get_load(repeat_i_rev(cores_n));
    this->current_total_load += load;
    this->current_load.emplace_back(load);
  }

  this->current_total_load = this->current_total_load / float(cores_n);

  this->label_tokenized->text = this->label->text;
  this->label_tokenized->replace_token("%percentage%", std::to_string((int)(this->current_total_load + 0.5f)) +"%");

  return true;
}

bool CpuModule::build(Builder *builder, const std::string& tag)
{
  if (tag == TAG_LABEL)
    builder->node(this->label_tokenized);
  else if (tag == TAG_BAR_LOAD)
    builder->node(this->bar_load, this->current_total_load);
  else if (tag == TAG_RAMP_LOAD)
    builder->node(this->ramp_load, this->current_total_load);
  else if (tag == TAG_RAMP_LOAD_PER_CORE) {
    int i = 0;
    for (auto &&load : this->current_load) {
      if (i++ > 0) builder->space(1);
      builder->node(this->ramp_load_per_core, load);
    }
    builder->node(builder->flush());

  } else return false;

  return true;
}

bool CpuModule::read_values()
{
  std::vector<std::unique_ptr<CpuTime>> cpu_times;

  try {
    std::ifstream in(PATH_CPU_INFO);
    std::string str;

    while (std::getline(in, str) && str.find("cpu") == 0) {
      // skip the accumulated line
      if (str.find("cpu ") == 0) continue;

      auto values = string::split(str, ' ');
      auto cpu = std::make_unique<CpuTime>();

      cpu->user = std::stoull(values[1].c_str(), 0, 10);
      cpu->nice = std::stoull(values[2].c_str(), 0, 10);
      cpu->system = std::stoull(values[3].c_str(), 0, 10);
      cpu->idle = std::stoull(values[4].c_str(), 0, 10);
      cpu->total = cpu->user + cpu->nice + cpu->system + cpu->idle;

      cpu_times.emplace_back(std::move(cpu));
    }
  } catch (std::ios_base::failure &e) {
    log_error(e.what());
  }

  this->prev_cpu_times.swap(this->cpu_times);
  this->cpu_times.swap(cpu_times);

  if (this->cpu_times.empty())
    log_error("Failed to read CPU values");

  return !this->cpu_times.empty();
}

float CpuModule::get_load(int core)
{
  if (this->cpu_times.size() == 0) return 0;
  if (this->prev_cpu_times.size() == 0) return 0;

  if (core < 0) return 0;
  if (core > (int) this->cpu_times.size() - 1) return 0;
  if (core > (int) this->prev_cpu_times.size() - 1) return 0;

  auto &last = this->cpu_times[core];
  auto &prev = this->prev_cpu_times[core];

  auto last_idle = last->idle;
  auto prev_idle = prev->idle;

  auto diff = last->total - prev->total;

  if (diff == 0) return 0;

  float load_percentage = 100.0f * (diff - (last_idle - prev_idle)) / diff;

  return math::cap<float>(load_percentage, 0, 100);
}
