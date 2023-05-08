#pragma once

#include "modules/meta/timer_module.hpp"
#include "modules/meta/types.hpp"
#include "settings.hpp"

POLYBAR_NS

namespace modules {
  enum class cpu_state { NORMAL = 0, WARN };
  struct cpu_time {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long steal;
    unsigned long long total;
  };

  using cpu_time_t = unique_ptr<cpu_time>;

  class cpu_module : public timer_module<cpu_module> {
   public:
    explicit cpu_module(const bar_settings&, string, const config&);

    bool update();
    string get_format() const;
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = CPU_TYPE;

   protected:
    bool read_values();
    float get_load(size_t core) const;

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_LABEL_WARN = "<label-warn>";
    static constexpr auto TAG_BAR_LOAD = "<bar-load>";
    static constexpr auto TAG_RAMP_LOAD = "<ramp-load>";
    static constexpr auto TAG_RAMP_LOAD_PER_CORE = "<ramp-coreload>";
    static constexpr auto FORMAT_WARN = "format-warn";

    label_t m_label;
    label_t m_labelwarn;
    progressbar_t m_barload;
    ramp_t m_rampload;
    ramp_t m_rampload_core;
    spacing_val m_ramp_padding{spacing_type::SPACE, 1U};

    vector<cpu_time_t> m_cputimes;
    vector<cpu_time_t> m_cputimes_prev;

    float m_totalwarn = 80;
    float m_total = 0;
    vector<float> m_load;
  };
}  // namespace modules

POLYBAR_NS_END
