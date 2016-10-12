#pragma once

#include "common.hpp"
#include "components/builder.hpp"
#include "components/config.hpp"
#include "components/types.hpp"
#include "drawtypes/label.hpp"
#include "utils/mixins.hpp"

LEMONBUDDY_NS

namespace drawtypes {
  class progressbar : public non_copyable_mixin<progressbar> {
   public:
    explicit progressbar(
        const bar_settings& bar, int width, string format, bool lazy_builder_closing)
        : m_builder(make_unique<builder>(bar, lazy_builder_closing))
        , m_format(format)
        , m_width(width) {}
    explicit progressbar(const bar_settings& bar, int width, bool lazy_builder_closing = true)
        : progressbar(bar, width, "<fill><indicator><empty>", lazy_builder_closing) {}

    void set_fill(icon_t&& fill) {
      m_fill = forward<decltype(fill)>(fill);
    }

    void set_empty(icon_t&& empty) {
      m_empty = forward<decltype(empty)>(empty);
    }

    void set_indicator(icon_t&& indicator) {
      m_indicator = forward<decltype(indicator)>(indicator);
    }

    void set_gradient(bool mode) {
      m_gradient = mode;
    }

    void set_colors(vector<string>&& colors) {
      m_colors = forward<decltype(colors)>(colors);
    }

    string output(float percentage) {
      if (m_colors.empty())
        m_colors.emplace_back(m_fill->m_foreground);

      int fill_width = m_width * percentage / 100.0f + 0.5f;
      int empty_width = m_width - fill_width;
      int color_step = m_width / m_colors.size() + 0.5f;

      auto output = string(m_format);

      if (m_indicator && *m_indicator) {
        if (empty_width == 1)
          empty_width = 0;
        else if (fill_width == 0)
          empty_width--;
        else
          fill_width--;
      }

      if (!m_gradient) {
        auto idx = static_cast<int>((m_colors.size() - 1) * percentage / 100.0f + 0.5f);
        m_fill->m_foreground = m_colors[idx];
        while (fill_width--) {
          m_builder->node(m_fill);
        }
      } else {
        int i = 0;
        for (auto color : m_colors) {
          i += 1;
          int j = 0;

          if ((i - 1) * color_step >= fill_width)
            break;

          m_fill->m_foreground = color;

          while (j++ < color_step && (i - 1) * color_step + j <= fill_width)
            m_builder->node(m_fill);
        }
      }

      output = string_util::replace_all(output, "%fill%", m_builder->flush());

      m_builder->node(m_indicator);
      output = string_util::replace_all(output, "%indicator%", m_builder->flush());

      while (empty_width--) m_builder->node(m_empty);
      output = string_util::replace_all(output, "%empty%", m_builder->flush());

      return output;
    }

   protected:
    unique_ptr<builder> m_builder;
    vector<string> m_colors;
    string m_format;
    unsigned int m_width;
    bool m_gradient = false;

    icon_t m_fill;
    icon_t m_empty;
    icon_t m_indicator;
  };

  using progressbar_t = shared_ptr<progressbar>;

  inline auto get_config_bar(const bar_settings& bar, const config& conf, string section,
      string name = "bar", bool lazy_builder_closing = true) {
    progressbar_t p;

    name = string_util::ltrim(string_util::rtrim(name, '>'), '<');

    auto width = conf.get<int>(section, name + "-width");
    auto format = conf.get<string>(section, name + "-format", "%fill%%indicator%%empty%");

    if (format.empty())
      p.reset(new progressbar(bar, width, lazy_builder_closing));
    else
      p.reset(new progressbar(bar, width, format, lazy_builder_closing));

    p->set_gradient(conf.get<bool>(section, name + "-gradient", true));
    p->set_colors(conf.get_list<string>(section, name + "-foreground", {}));
    p->set_indicator(get_config_icon(
        conf, section, name + "-indicator", format.find("%indicator%") != string::npos, ""));
    p->set_fill(
        get_config_icon(conf, section, name + "-fill", format.find("%fill%") != string::npos, ""));
    p->set_empty(get_config_icon(
        conf, section, name + "-empty", format.find("%empty%") != string::npos, ""));

    return p;
  }
}

LEMONBUDDY_NS_END
