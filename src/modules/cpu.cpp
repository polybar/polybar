#include "modules/cpu.hpp"

#include <fstream>
#include <istream>

#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta/base.inl"
#include "utils/math.hpp"

POLYBAR_NS

namespace modules {
  template class module<cpu_module>;

  cpu_module::cpu_module(const bar_settings& bar, string name_, const config& config)
      : timer_module<cpu_module>(bar, move(name_), config) {
    set_interval(1s);
    m_totalwarn = m_conf.get(name(), "warn-percentage", m_totalwarn);
    m_ramp_padding = m_conf.get(name(), "ramp-coreload-spacing", m_ramp_padding);

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_BAR_LOAD, TAG_RAMP_LOAD, TAG_RAMP_LOAD_PER_CORE});
    m_formatter->add_optional(FORMAT_WARN, {TAG_LABEL_WARN, TAG_BAR_LOAD, TAG_RAMP_LOAD, TAG_RAMP_LOAD_PER_CORE});

    // warmup cpu times
    read_values();
    read_values();

    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%percentage%%");
    }
    if (m_formatter->has(TAG_LABEL_WARN)) {
      m_labelwarn = load_optional_label(m_conf, name(), TAG_LABEL_WARN, "%percentage%%");
    }
    if (m_formatter->has(TAG_BAR_LOAD)) {
      m_barload = load_progressbar(m_bar, m_conf, name(), TAG_BAR_LOAD);
    }
    if (m_formatter->has(TAG_RAMP_LOAD)) {
      m_rampload = load_ramp(m_conf, name(), TAG_RAMP_LOAD);
    }
    if (m_formatter->has(TAG_RAMP_LOAD_PER_CORE)) {
      m_rampload_core = load_ramp(m_conf, name(), TAG_RAMP_LOAD_PER_CORE);
    }
  }

  bool cpu_module::update() {
    if (!read_values()) {
      return false;
    }

    m_total = 0.0f;
    m_load.clear();

    auto cores_n = m_cputimes.size();
    if (!cores_n) {
      return false;
    }

    vector<string> percentage_cores;
    for (size_t i = 0; i < cores_n; i++) {
      auto load = get_load(i);
      m_total += load;
      m_load.emplace_back(load);

      if (m_label || m_labelwarn) {
        percentage_cores.emplace_back(to_string(static_cast<int>(load + 0.5)));
      }
    }

    m_total = m_total / static_cast<float>(cores_n);

    const auto replace_tokens = [&](label_t& label) {
      label->reset_tokens();
      label->replace_token("%percentage%", to_string(static_cast<int>(m_total + 0.5)));
      label->replace_token(
          "%percentage-sum%", to_string(static_cast<int>(m_total * static_cast<float>(cores_n) + 0.5)));
      label->replace_token("%percentage-cores%", string_util::join(percentage_cores, "% ") + "%");

      for (size_t i = 0; i < percentage_cores.size(); i++) {
        label->replace_token("%percentage-core" + to_string(i + 1) + "%", percentage_cores[i]);
      }
    };

    if (m_label) {
      replace_tokens(m_label);
    }
    if (m_labelwarn) {
      replace_tokens(m_labelwarn);
    }

    return true;
  }

  string cpu_module::get_format() const {
    if (m_total >= m_totalwarn && m_formatter->has_format(FORMAT_WARN)) {
      return FORMAT_WARN;
    } else {
      return DEFAULT_FORMAT;
    }
  }

  bool cpu_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else if (tag == TAG_LABEL_WARN) {
      builder->node(m_labelwarn);
    } else if (tag == TAG_BAR_LOAD) {
      builder->node(m_barload->output(m_total));
    } else if (tag == TAG_RAMP_LOAD) {
      builder->node(m_rampload->get_by_percentage_with_borders(m_total, 0.0f, m_totalwarn));
    } else if (tag == TAG_RAMP_LOAD_PER_CORE) {
      auto i = 0;
      for (auto&& load : m_load) {
        if (i++ > 0) {
          builder->spacing(m_ramp_padding);
        }
        builder->node(m_rampload_core->get_by_percentage_with_borders(load, 0.0f, m_totalwarn));
      }
      builder->node(builder->flush());
    } else {
      return false;
    }
    return true;
  }

  bool cpu_module::read_values() {
    m_cputimes_prev.swap(m_cputimes);
    m_cputimes.clear();

    try {
      std::ifstream in(PATH_CPU_INFO);
      string str;

      while (std::getline(in, str) && str.compare(0, 3, "cpu") == 0) {
        // skip line with accumulated value
        if (str.compare(0, 4, "cpu ") == 0) {
          continue;
        }

        auto values = string_util::split(str, ' ');

        m_cputimes.emplace_back(new cpu_time);
        m_cputimes.back()->user = std::stoull(values[1], nullptr, 10);
        m_cputimes.back()->nice = std::stoull(values[2], nullptr, 10);
        m_cputimes.back()->system = std::stoull(values[3], nullptr, 10);
        m_cputimes.back()->idle = std::stoull(values[4], nullptr, 10);
        m_cputimes.back()->steal = std::stoull(values[8], nullptr, 10);
        m_cputimes.back()->total = m_cputimes.back()->user + m_cputimes.back()->nice + m_cputimes.back()->system +
                                   m_cputimes.back()->idle + m_cputimes.back()->steal;
      }
    } catch (const std::ios_base::failure& e) {
      m_log.err("Failed to read CPU values (what: %s)", e.what());
    }

    return !m_cputimes.empty();
  }

  float cpu_module::get_load(size_t core) const {
    if (m_cputimes.empty() || m_cputimes_prev.empty()) {
      return 0;
    } else if (core >= m_cputimes.size() || core >= m_cputimes_prev.size()) {
      return 0;
    }

    auto& last = m_cputimes[core];
    auto& prev = m_cputimes_prev[core];

    auto last_idle = last->idle;
    auto prev_idle = prev->idle;

    auto diff = last->total - prev->total;

    if (diff == 0) {
      return 0;
    }

    float percentage = 100.0f * (diff - (last_idle - prev_idle)) / diff;

    return math_util::cap<float>(percentage, 0, 100);
  }
}  // namespace modules

POLYBAR_NS_END
