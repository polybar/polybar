#include "modules/memory.hpp"

LEMONBUDDY_NS

namespace modules {
  void memory_module::setup() {
    m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 1));

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_BAR_USED, TAG_BAR_FREE});

    if (m_formatter->has(TAG_BAR_USED))
      m_bars[memtype::USED] = load_progressbar(m_bar, m_conf, name(), TAG_BAR_USED);
    if (m_formatter->has(TAG_BAR_FREE))
      m_bars[memtype::FREE] = load_progressbar(m_bar, m_conf, name(), TAG_BAR_FREE);
    if (m_formatter->has(TAG_LABEL))
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%percentage_used%");
  }

  bool memory_module::update() {
    float kb_total;
    float kb_avail;
    // long kb_free;

    try {
      std::ifstream in(PATH_MEMORY_INFO);
      std::stringstream buffer;
      string str, rdbuf;
      int i = 0;

      in.exceptions(in.failbit);

      buffer.imbue(std::locale::classic());

      while (std::getline(in, str) && i++ < 3) {
        size_t off = str.find_first_of("1234567890", str.find(':'));
        buffer << std::strtol(&str[off], 0, 10) << std::endl;
      }

      buffer >> rdbuf;
      kb_total = std::atol(rdbuf.c_str());
      buffer >> rdbuf;  // kb_free = std::atol(rdbuf.c_str());
      buffer >> rdbuf;
      kb_avail = std::atol(rdbuf.c_str());
    } catch (const std::ios_base::failure& e) {
      kb_total = 0;
      // kb_free = 0;
      kb_avail = 0;
      m_log.err("Failed to read memory values (what: %s)", e.what());
    }

    if (kb_total > 0)
      m_perc[memtype::FREE] = (kb_avail / kb_total) * 100.0f + 0.5f;
    else
      m_perc[memtype::FREE] = 0;

    m_perc[memtype::USED] = 100 - m_perc[memtype::FREE];

    // replace tokens
    if (m_label) {
      m_label->reset_tokens();

      auto replace_unit = [](label_t& label, string token, float value, string unit) {
        auto formatted = string_util::from_stream(
            stringstream() << std::setprecision(2) << std::fixed << value << " " << unit);
        label->replace_token(token, formatted);
      };

      replace_unit(m_label, "%gb_used%", (kb_total - kb_avail) / 1024 / 1024, "GB");
      replace_unit(m_label, "%gb_free%", kb_avail / 1024 / 1024, "GB");
      replace_unit(m_label, "%gb_total%", kb_total / 1024 / 1024, "GB");
      replace_unit(m_label, "%mb_used%", (kb_total - kb_avail) / 1024, "MB");
      replace_unit(m_label, "%mb_free%", kb_avail / 1024, "MB");
      replace_unit(m_label, "%mb_total%", kb_total / 1024, "MB");

      m_label->replace_token("%percentage_used%", to_string(m_perc[memtype::USED]) + "%");
      m_label->replace_token("%percentage_free%", to_string(m_perc[memtype::FREE]) + "%");
    }

    return true;
  }

  bool memory_module::build(builder* builder, string tag) const {
    if (tag == TAG_BAR_USED)
      builder->node(m_bars.at(memtype::USED)->output(m_perc.at(memtype::USED)));
    else if (tag == TAG_BAR_FREE)
      builder->node(m_bars.at(memtype::FREE)->output(m_perc.at(memtype::FREE)));
    else if (tag == TAG_LABEL)
      builder->node(m_label);
    else
      return false;
    return true;
  }
}

LEMONBUDDY_NS_END
