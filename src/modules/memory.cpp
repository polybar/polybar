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

  memory_module::memory_module(const bar_settings& bar, string name_, const config& config)
      : timer_module<memory_module>(bar, move(name_), config) {
    set_interval(1s);
    m_perc_memused_warn = m_conf.get(name(), "warn-percentage", 90);

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_BAR_USED, TAG_BAR_FREE, TAG_RAMP_USED, TAG_RAMP_FREE,
                                                 TAG_BAR_SWAP_USED, TAG_BAR_SWAP_FREE, TAG_RAMP_SWAP_USED, TAG_RAMP_SWAP_FREE});
    m_formatter->add_optional(FORMAT_WARN, {TAG_LABEL_WARN, TAG_BAR_USED, TAG_BAR_FREE, TAG_RAMP_USED, TAG_RAMP_FREE,
                                                 TAG_BAR_SWAP_USED, TAG_BAR_SWAP_FREE, TAG_RAMP_SWAP_USED, TAG_RAMP_SWAP_FREE});

    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%percentage_used%%");
    }
    if (m_formatter->has(TAG_LABEL_WARN)) {
      m_labelwarn = load_optional_label(m_conf, name(), TAG_LABEL_WARN, "%percentage_used%%");
    }
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
    const auto replace_tokens = [&](label_t& label) {
      label->reset_tokens();
      label->replace_token("%gb_used%", string_util::filesize_gib(kb_total - kb_avail, 2, m_bar.locale));
      label->replace_token("%gb_free%", string_util::filesize_gib(kb_avail, 2, m_bar.locale));
      label->replace_token("%gb_total%", string_util::filesize_gib(kb_total, 2, m_bar.locale));
      label->replace_token("%mb_used%", string_util::filesize_mib(kb_total - kb_avail, 0, m_bar.locale));
      label->replace_token("%mb_free%", string_util::filesize_mib(kb_avail, 0, m_bar.locale));
      label->replace_token("%mb_total%", string_util::filesize_mib(kb_total, 0, m_bar.locale));
      label->replace_token("%percentage_used%", to_string(m_perc_memused));
      label->replace_token("%percentage_free%", to_string(m_perc_memfree));
      label->replace_token("%percentage_swap_used%", to_string(m_perc_swap_used));
      label->replace_token("%percentage_swap_free%", to_string(m_perc_swap_free));
      label->replace_token("%mb_swap_total%", string_util::filesize_mib(kb_swap_total, 0, m_bar.locale));
      label->replace_token("%mb_swap_free%", string_util::filesize_mib(kb_swap_free, 0, m_bar.locale));
      label->replace_token("%mb_swap_used%", string_util::filesize_mib(kb_swap_total - kb_swap_free, 0, m_bar.locale));
      label->replace_token("%gb_swap_total%", string_util::filesize_gib(kb_swap_total, 2, m_bar.locale));
      label->replace_token("%gb_swap_free%", string_util::filesize_gib(kb_swap_free, 2, m_bar.locale));
      label->replace_token("%gb_swap_used%", string_util::filesize_gib(kb_swap_total - kb_swap_free, 2, m_bar.locale));
      label->replace_token("%used%", string_util::filesize_gib_mib(kb_total - kb_avail, 0, 2, m_bar.locale));
      label->replace_token("%free%", string_util::filesize_gib_mib(kb_avail, 0, 2, m_bar.locale));
      label->replace_token("%total%", string_util::filesize_gib_mib(kb_total, 0, 2, m_bar.locale));
      label->replace_token("%swap_total%", string_util::filesize_gib_mib(kb_swap_total, 0, 2, m_bar.locale));
      label->replace_token("%swap_free%", string_util::filesize_gib_mib(kb_swap_free, 0, 2, m_bar.locale));
      label->replace_token("%swap_used%", string_util::filesize_gib_mib(kb_swap_total - kb_swap_free, 0, 2, m_bar.locale));
    };

    if (m_label) {
      replace_tokens(m_label);
    }

    if (m_labelwarn) {
      replace_tokens(m_labelwarn);
    }

    return true;
  }

  string memory_module::get_format() const {
    if (m_perc_memused>= m_perc_memused_warn && m_formatter->has_format(FORMAT_WARN)) {
      return FORMAT_WARN;
    } else {
      return DEFAULT_FORMAT;
    }
  }

  bool memory_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_BAR_USED) {
      builder->node(m_bar_memused->output(m_perc_memused));
    } else if (tag == TAG_BAR_FREE) {
      builder->node(m_bar_memfree->output(m_perc_memfree));
    } else if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else if (tag == TAG_LABEL_WARN) {
      builder->node(m_labelwarn);
    } else if (tag == TAG_RAMP_FREE) {
      builder->node(m_ramp_memfree->get_by_percentage_with_borders(m_perc_memfree, 0, m_perc_memused_warn));
    } else if (tag == TAG_RAMP_USED) {
      builder->node(m_ramp_memused->get_by_percentage_with_borders(m_perc_memused, 0, m_perc_memused_warn));
    } else if (tag == TAG_BAR_SWAP_USED) {
      builder->node(m_bar_swapused->output(m_perc_swap_used));
    } else if (tag == TAG_BAR_SWAP_FREE) {
      builder->node(m_bar_swapfree->output(m_perc_swap_free));
    } else if (tag == TAG_RAMP_SWAP_FREE) {
      builder->node(m_ramp_swapfree->get_by_percentage_with_borders(m_perc_swap_free, 0, m_perc_memused_warn));
    } else if (tag == TAG_RAMP_SWAP_USED) {
      builder->node(m_ramp_swapused->get_by_percentage_with_borders(m_perc_swap_used, 0, m_perc_memused_warn));
    } else {
      return false;
    }
    return true;
  }
}

POLYBAR_NS_END
