#include <fstream>
#include <iomanip>
#include <istream>

#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "modules/memory.hpp"
#include "utils/math.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<memory_module>;

  memory_module::memory_module(const bar_settings& bar, string name_) : timer_module<memory_module>(bar, move(name_)) {
    m_interval = m_conf.get<decltype(m_interval)>(name(), "interval", 1s);

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_BAR_USED, TAG_BAR_FREE});

    if (m_formatter->has(TAG_BAR_USED)) {
      m_bar_memused = load_progressbar(m_bar, m_conf, name(), TAG_BAR_USED);
    }
    if (m_formatter->has(TAG_BAR_FREE)) {
      m_bar_memfree = load_progressbar(m_bar, m_conf, name(), TAG_BAR_FREE);
    }
    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%percentage_used%");
    }
  }

  bool memory_module::update() {
    unsigned long long kb_total{0ULL};
    unsigned long long kb_avail{0ULL};
    unsigned long long kb_free{0ULL};

    try {
      std::ifstream in(PATH_MEMORY_INFO);
      std::stringstream buffer;
      string str;

      buffer.imbue(std::locale::classic());

      for (int i = 3; i > 0 && std::getline(in, str); i--) {
        size_t off = str.find_first_of("1234567890", str.find(':'));
        if (off != string::npos && str.size() > off) {
          buffer << std::strtoull(&str[off], nullptr, 10) << std::endl;
        }
      }

      buffer >> kb_total;
      buffer >> kb_free;
      buffer >> kb_avail;
    } catch (const std::exception& err) {
      m_log.err("Failed to read memory values (what: %s)", err.what());
    }

    m_perc_memfree = math_util::percentage(kb_avail, kb_total);
    m_perc_memused = 100 - m_perc_memfree;

    // replace tokens
    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%gb_used%", string_util::filesize_gb(kb_total - kb_avail, 2, m_bar.locale));
      m_label->replace_token("%gb_free%", string_util::filesize_gb(kb_avail, 2, m_bar.locale));
      m_label->replace_token("%gb_total%", string_util::filesize_gb(kb_total, 2, m_bar.locale));
      m_label->replace_token("%mb_used%", string_util::filesize_mb(kb_total - kb_avail, 2, m_bar.locale));
      m_label->replace_token("%mb_free%", string_util::filesize_mb(kb_avail, 2, m_bar.locale));
      m_label->replace_token("%mb_total%", string_util::filesize_mb(kb_total, 2, m_bar.locale));
      m_label->replace_token("%percentage_used%", to_string(m_perc_memused) + "%");
      m_label->replace_token("%percentage_free%", to_string(m_perc_memfree) + "%");
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
    } else {
      return false;
    }
    return true;
  }
}

POLYBAR_NS_END
