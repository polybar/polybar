#pragma once

#include <memory>
#include <string>
#include <vector>

#include "modules/base.hpp"
#include "drawtypes/icon.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/ramp.hpp"

namespace modules
{
  struct CpuTime
  {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long total;
  };

  DefineModule(CpuModule, TimerModule)
  {
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR_LOAD = "<bar-load>";
    static constexpr auto TAG_RAMP_LOAD = "<ramp-load>";
    static constexpr auto TAG_RAMP_LOAD_PER_CORE = "<ramp-load_per_core>";

    std::vector<std::unique_ptr<CpuTime>> cpu_times;
    std::vector<std::unique_ptr<CpuTime>> prev_cpu_times;

    std::unique_ptr<drawtypes::Bar> bar_load;
    std::unique_ptr<drawtypes::Ramp> ramp_load;
    std::unique_ptr<drawtypes::Ramp> ramp_load_per_core;
    std::unique_ptr<drawtypes::Label> label;
    std::unique_ptr<drawtypes::Label> label_tokenized;

    float current_total_load = 0;
    std::vector<float> current_load;

    bool read_values();
    float get_load(int core);

    public:
      explicit CpuModule(std::string name);

      bool update();
      bool build(Builder *builder, std::string tag);
  };
}
