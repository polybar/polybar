#pragma once

#include <istream>

#include "config.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  struct cpu_time {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long total;
  };

  using cpu_time_t = unique_ptr<cpu_time>;

  class cpu_module : public timer_module<cpu_module> {
   public:
    using timer_module::timer_module;

    void setup() {
      m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 1));

      m_formatter->add(DEFAULT_FORMAT, TAG_LABEL,
          {TAG_LABEL, TAG_BAR_LOAD, TAG_RAMP_LOAD, TAG_RAMP_LOAD_PER_CORE});

      if (m_formatter->has(TAG_BAR_LOAD))
        m_barload = get_config_bar(m_bar, m_conf, name(), TAG_BAR_LOAD);
      if (m_formatter->has(TAG_RAMP_LOAD))
        m_rampload = get_config_ramp(m_conf, name(), TAG_RAMP_LOAD);
      if (m_formatter->has(TAG_RAMP_LOAD_PER_CORE))
        m_rampload_core = get_config_ramp(m_conf, name(), TAG_RAMP_LOAD_PER_CORE);
      if (m_formatter->has(TAG_LABEL)) {
        m_label = get_optional_config_label(m_conf, name(), TAG_LABEL, "%percentage%");
        m_tokenized = m_label->clone();
      }

      // warmup
      read_values();
      read_values();
    }

    bool update() {
      if (!read_values())
        return false;

      m_total = 0.0f;
      m_load.clear();

      auto cores_n = m_cputimes.size();

      if (!cores_n)
        return false;

      for (size_t i = 0; i < cores_n; i++) {
        auto load = get_load(i);
        m_total += load;
        m_load.emplace_back(load);
      }

      m_total = m_total / static_cast<float>(cores_n);

      if (m_tokenized) {
        m_tokenized->m_text = m_label->m_text;
        m_tokenized->replace_token(
            "%percentage%", to_string(static_cast<int>(m_total + 0.5f)) + "%");
      }

      return true;
    }

    bool build(builder* builder, string tag) {
      if (tag == TAG_LABEL)
        builder->node(m_tokenized);
      else if (tag == TAG_BAR_LOAD)
        builder->node(m_barload->output(m_total));
      else if (tag == TAG_RAMP_LOAD)
        builder->node(m_rampload->get_by_percentage(m_total));
      else if (tag == TAG_RAMP_LOAD_PER_CORE) {
        auto i = 0;
        for (auto&& load : m_load) {
          if (i++ > 0)
            builder->space(1);
          builder->node(m_rampload_core->get_by_percentage(load));
        }
        builder->node(builder->flush());
      } else
        return false;
      return true;
    }

   protected:
    bool read_values() {
      m_cputimes_prev.swap(m_cputimes);
      m_cputimes.clear();

      try {
        std::ifstream in(PATH_CPU_INFO);
        string str;

        while (std::getline(in, str) && str.compare(0, 3, "cpu") == 0) {
          // skip line with accumulated value
          if (str.compare(0, 4, "cpu ") == 0)
            continue;

          auto values = string_util::split(str, ' ');

          m_cputimes.emplace_back(new cpu_time);
          m_cputimes.back()->user = std::stoull(values[1].c_str(), 0, 10);
          m_cputimes.back()->nice = std::stoull(values[2].c_str(), 0, 10);
          m_cputimes.back()->system = std::stoull(values[3].c_str(), 0, 10);
          m_cputimes.back()->idle = std::stoull(values[4].c_str(), 0, 10);
          m_cputimes.back()->total = m_cputimes.back()->user + m_cputimes.back()->nice +
                                     m_cputimes.back()->system + m_cputimes.back()->idle;
        }
      } catch (const std::ios_base::failure& e) {
        m_log.err("Failed to read CPU values (what: %s)", e.what());
      }

      return !m_cputimes.empty();
    }

    float get_load(size_t core) {
      if (m_cputimes.size() == 0)
        return 0;
      else if (m_cputimes_prev.size() == 0)
        return 0;
      else if (!core)
        return 0;
      else if (core > m_cputimes.size() - 1)
        return 0;
      else if (core > m_cputimes_prev.size() - 1)
        return 0;

      auto& last = m_cputimes[core];
      auto& prev = m_cputimes_prev[core];

      auto last_idle = last->idle;
      auto prev_idle = prev->idle;

      auto diff = last->total - prev->total;

      if (diff == 0)
        return 0;

      float percentage = 100.0f * (diff - (last_idle - prev_idle)) / diff;

      return math_util::cap<float>(percentage, 0, 100);
    }

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR_LOAD = "<bar-load>";
    static constexpr auto TAG_RAMP_LOAD = "<ramp-load>";
    static constexpr auto TAG_RAMP_LOAD_PER_CORE = "<ramp-coreload>";

    progressbar_t m_barload;
    ramp_t m_rampload;
    ramp_t m_rampload_core;
    label_t m_label;
    label_t m_tokenized;

    vector<cpu_time_t> m_cputimes;
    vector<cpu_time_t> m_cputimes_prev;

    float m_total = 0;
    vector<float> m_load;
  };
}

LEMONBUDDY_NS_END
