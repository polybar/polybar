#include <fstream>
#include <iomanip>
#include <istream>

#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/memory.hpp"
#include "utils/math.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<memory_module>;

  memory_module::memory_module(const bar_settings& bar, string name_) : timer_module<memory_module>(bar, move(name_)) {
    m_interval = m_conf.get<decltype(m_interval)>(name(), "interval", 1s);

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_BAR_USED, TAG_BAR_FREE, TAG_RAMP_USED, TAG_RAMP_FREE,
                                                 TAG_BAR_SWAP_USED, TAG_BAR_SWAP_FREE, TAG_RAMP_SWAP_USED, TAG_RAMP_SWAP_FREE});

    if (m_formatter->has(TAG_BAR_USED)) {
      m_bar_memused = load_progressbar(m_bar, m_conf, name(), TAG_BAR_USED);
    }
    if (m_formatter->has(TAG_BAR_FREE)) {
      m_bar_memfree = load_progressbar(m_bar, m_conf, name(), TAG_BAR_FREE);
    }
    if(m_formatter->has(TAG_RAMP_USED)) {
      m_ramp_memused = load_ramp(m_conf, name(), TAG_RAMP_USED);
    }
    if(m_formatter->has(TAG_RAMP_FREE)) {
      m_ramp_memfree = load_ramp(m_conf, name(), TAG_RAMP_FREE);
    }
    if (m_formatter->has(TAG_BAR_SWAP_USED)) {
      m_bar_swapused = load_progressbar(m_bar, m_conf, name(), TAG_BAR_SWAP_USED);
    }
    if (m_formatter->has(TAG_BAR_SWAP_FREE)) {
      m_bar_swapfree = load_progressbar(m_bar, m_conf, name(), TAG_BAR_SWAP_FREE);
    }
    if(m_formatter->has(TAG_RAMP_SWAP_USED)) {
      m_ramp_swapused = load_ramp(m_conf, name(), TAG_RAMP_SWAP_USED);
    }
    if(m_formatter->has(TAG_RAMP_SWAP_FREE)) {
      m_ramp_swapfree = load_ramp(m_conf, name(), TAG_RAMP_SWAP_FREE);
    }

    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%percentage_used%%");
    }
  }

  bool memory_module::update() {
    unsigned long long kb_total{0ULL};
    unsigned long long kb_avail{0ULL};
    unsigned long long kb_swap_total{0ULL};
    unsigned long long kb_swap_free{0ULL};

    try {
      std::ifstream meminfo(PATH_MEMORY_INFO);
      std::map<std::string, unsigned long long int> parsed;

      std::string line;
      while (std::getline(meminfo, line)) {
        size_t sep_off = line.find(':');
        size_t value_off = line.find_first_of("123456789", sep_off);

        if (sep_off == std::string::npos || value_off == std::string::npos) continue;

        std::string id = line.substr(0, sep_off);
        unsigned long long int value = std::strtoull(&line[value_off], nullptr, 10);
        parsed[id] = value;
      }

      kb_total = parsed["MemTotal"];
      kb_swap_total = parsed["SwapTotal"];
      kb_swap_free = parsed["SwapFree"];

      // newer kernels (3.4+) have an accurate available memory field,
      // see https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=34e431b0ae398fc54ea69ff85ec700722c9da773
      // for details
      if (parsed.count("MemAvailable")) {
        kb_avail = parsed["MemAvailable"];
      } else {
        // old kernel; give a best-effort approximation of available memory
        kb_avail = parsed["MemFree"] + parsed["Buffers"] + parsed["Cached"] + parsed["SReclaimable"] - parsed["Shmem"];
      }
    } catch (const std::exception& err) {
      m_log.err("Failed to read memory values (what: %s)", err.what());
    }

    m_perc_memfree = math_util::percentage(kb_avail, kb_total);
    m_perc_memused = 100 - m_perc_memfree;
    m_perc_swap_free = math_util::percentage(kb_swap_free, kb_swap_total);
    m_perc_swap_used = 100 - m_perc_swap_free;

    // replace tokens
    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%gb_used%", string_util::filesize_gb(kb_total - kb_avail, 2, m_bar.locale));
      m_label->replace_token("%gb_free%", string_util::filesize_gb(kb_avail, 2, m_bar.locale));
      m_label->replace_token("%gb_total%", string_util::filesize_gb(kb_total, 2, m_bar.locale));
      m_label->replace_token("%mb_used%", string_util::filesize_mb(kb_total - kb_avail, 0, m_bar.locale));
      m_label->replace_token("%mb_free%", string_util::filesize_mb(kb_avail, 0, m_bar.locale));
      m_label->replace_token("%mb_total%", string_util::filesize_mb(kb_total, 0, m_bar.locale));
      m_label->replace_token("%percentage_used%", to_string(m_perc_memused));
      m_label->replace_token("%percentage_free%", to_string(m_perc_memfree));
      m_label->replace_token("%percentage_swap_used%", to_string(m_perc_swap_used));
      m_label->replace_token("%percentage_swap_free%", to_string(m_perc_swap_free));
      m_label->replace_token("%mb_swap_total%", string_util::filesize_mb(kb_swap_total, 0, m_bar.locale));
      m_label->replace_token("%mb_swap_free%", string_util::filesize_mb(kb_swap_free, 0, m_bar.locale));
      m_label->replace_token("%mb_swap_used%", string_util::filesize_mb(kb_swap_total - kb_swap_free, 0, m_bar.locale));
      m_label->replace_token("%gb_swap_total%", string_util::filesize_gb(kb_swap_total, 2, m_bar.locale));
      m_label->replace_token("%gb_swap_free%", string_util::filesize_gb(kb_swap_free, 2, m_bar.locale));
      m_label->replace_token("%gb_swap_used%", string_util::filesize_gb(kb_swap_total - kb_swap_free, 2, m_bar.locale));
    }

    return true;
  }

  bool memory_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_BAR_USED) {
      builder->node(m_bar_memused->output(m_perc_memused));
    } else if (tag == TAG_BAR_FREE) {
      builder->node(m_bar_memfree->output(m_perc_memfree));
    } else if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else if (tag == TAG_RAMP_FREE) {
      builder->node(m_ramp_memfree->get_by_percentage(m_perc_memfree));
    } else if (tag == TAG_RAMP_USED) {
      builder->node(m_ramp_memused->get_by_percentage(m_perc_memused));
    } else if (tag == TAG_BAR_SWAP_USED) {
      builder->node(m_bar_swapused->output(m_perc_swap_used));
    } else if (tag == TAG_BAR_SWAP_FREE) {
      builder->node(m_bar_swapfree->output(m_perc_swap_free));
    } else if (tag == TAG_RAMP_SWAP_FREE) {
      builder->node(m_ramp_swapfree->get_by_percentage(m_perc_swap_free));
    } else if (tag == TAG_RAMP_SWAP_USED) {
      builder->node(m_ramp_swapused->get_by_percentage(m_perc_swap_used));
    } else {
      return false;
    }
    return true;
  }
}

POLYBAR_NS_END
