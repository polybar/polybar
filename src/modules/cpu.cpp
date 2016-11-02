#include "modules/cpu.hpp"
#include "utils/math.hpp"

LEMONBUDDY_NS

namespace modules {
  void cpu_module::setup() {
    m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 1));

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL,
        {TAG_LABEL, TAG_BAR_LOAD, TAG_RAMP_LOAD, TAG_RAMP_LOAD_PER_CORE});

    if (m_formatter->has(TAG_BAR_LOAD))
      m_barload = load_progressbar(m_bar, m_conf, name(), TAG_BAR_LOAD);
    if (m_formatter->has(TAG_RAMP_LOAD))
      m_rampload = load_ramp(m_conf, name(), TAG_RAMP_LOAD);
    if (m_formatter->has(TAG_RAMP_LOAD_PER_CORE))
      m_rampload_core = load_ramp(m_conf, name(), TAG_RAMP_LOAD_PER_CORE);
    if (m_formatter->has(TAG_LABEL))
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%percentage%");

    // warmup
    read_values();
    read_values();
  }

  bool cpu_module::update() {
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

    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%percentage%", to_string(static_cast<int>(m_total + 0.5f)) + "%");
    }

    return true;
  }

  bool cpu_module::build(builder* builder, string tag) const {
    if (tag == TAG_LABEL)
      builder->node(m_label);
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

  bool cpu_module::read_values() {
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

  float cpu_module::get_load(size_t core) const {
    if (m_cputimes.empty() || m_cputimes_prev.empty())
      return 0;
    else if (core >= m_cputimes.size() || core >= m_cputimes_prev.size())
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
}

LEMONBUDDY_NS_END
