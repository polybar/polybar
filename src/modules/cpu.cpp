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

  cpu_module::cpu_module(const bar_settings& bar, string name_) : timer_module<cpu_module>(bar, move(name_)) {
    m_interval = m_conf.get<decltype(m_interval)>(name(), "interval", 1s);

    m_ramp_padding = m_conf.get<decltype(m_ramp_padding)>(name(), "ramp-coreload-spacing", 1);

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_BAR_LOAD, TAG_RAMP_LOAD, TAG_RAMP_LOAD_PER_CORE});

    m_adapter = make_unique<cpu_adapter>();

    if (m_formatter->has(TAG_BAR_LOAD)) {
      m_barload = load_progressbar(m_bar, m_conf, name(), TAG_BAR_LOAD);
    }
    if (m_formatter->has(TAG_RAMP_LOAD)) {
      m_rampload = load_ramp(m_conf, name(), TAG_RAMP_LOAD);
    }
    if (m_formatter->has(TAG_RAMP_LOAD_PER_CORE)) {
      m_rampload_core = load_ramp(m_conf, name(), TAG_RAMP_LOAD_PER_CORE);
    }
    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%percentage%%");
    }
  }

  bool cpu_module::update() {
    if (!m_adapter->update()) {
      // TODO maybe there should be cooldown period so that if an error is
      // permanent, we don't retry 40 times per second.
      if(m_adapter->get_state() != cpu_state::RUNNING) {
        m_adapter->start();
      }

      return false;
    }

    m_total = 0.0f;
    m_load.clear();

    auto cores_n = m_adapter->num_cores();
    if (!cores_n) {
      return false;
    }

    m_load = m_adapter->get_load();
    vector<string> percentage_cores;
    for (auto load : m_load) {
      float load_perc = math_util::percentage(load, 1.0f);
      m_total += load_perc;

      if (m_label) {
        percentage_cores.emplace_back(to_string(static_cast<int>(load_perc + 0.5)));
      }
    }

    m_total = m_total / static_cast<float>(cores_n);

    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%percentage%", to_string(static_cast<int>(m_total + 0.5)));
      m_label->replace_token(
          "%percentage-sum%", to_string(static_cast<int>(m_total * static_cast<float>(cores_n) + 0.5)));
      m_label->replace_token("%percentage-cores%", string_util::join(percentage_cores, "% ") + "%");

      for (size_t i = 0; i < percentage_cores.size(); i++) {
        m_label->replace_token("%percentage-core" + to_string(i + 1) + "%", percentage_cores[i]);
      }
    }

    return true;
  }

  bool cpu_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else if (tag == TAG_BAR_LOAD) {
      builder->node(m_barload->output(m_total));
    } else if (tag == TAG_RAMP_LOAD) {
      builder->node(m_rampload->get_by_percentage(m_total));
    } else if (tag == TAG_RAMP_LOAD_PER_CORE) {
      auto i = 0;
      for (auto&& load : m_load) {
        if (i++ > 0) {
          builder->space(m_ramp_padding);
        }
        builder->node(m_rampload_core->get_by_percentage(math_util::percentage(load, 1.0f)));
      }
      builder->node(builder->flush());
    } else {
      return false;
    }
    return true;
  }
}  // namespace modules

POLYBAR_NS_END
