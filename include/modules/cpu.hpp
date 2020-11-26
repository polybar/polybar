#pragma once

#include "modules/cpu_adapter.hpp"
#include "modules/meta/timer_module.hpp"
#include "settings.hpp"

POLYBAR_NS

namespace modules {
  class cpu_module : public timer_module<cpu_module> {
   public:
    explicit cpu_module(const bar_settings&, string);

    bool update();
    bool build(builder* builder, const string& tag) const;

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR_LOAD = "<bar-load>";
    static constexpr auto TAG_RAMP_LOAD = "<ramp-load>";
    static constexpr auto TAG_RAMP_LOAD_PER_CORE = "<ramp-coreload>";

    cpu_adapter_t m_adapter;

    progressbar_t m_barload;
    ramp_t m_rampload;
    ramp_t m_rampload_core;
    label_t m_label;
    int m_ramp_padding;

    float m_total = 0;
    vector<float> m_load;
  };
}  // namespace modules

POLYBAR_NS_END
