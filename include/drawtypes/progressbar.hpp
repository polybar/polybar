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
        const bar_settings& bar, int width, string format, bool lazy_builder_closing = true)
        : m_builder(make_unique<builder>(bar, lazy_builder_closing))
        , m_format(format)
        , m_width(width) {}

    void set_fill(icon_t&& fill);
    void set_empty(icon_t&& empty);
    void set_indicator(icon_t&& indicator);
    void set_gradient(bool mode);
    void set_colors(vector<string>&& colors);

    string output(float percentage);

   protected:
    void fill(unsigned int perc, unsigned int fill_width);

   private:
    unique_ptr<builder> m_builder;
    vector<string> m_colors;
    string m_format;
    unsigned int m_width;
    unsigned int m_colorstep = 1;
    bool m_gradient = false;

    icon_t m_fill;
    icon_t m_empty;
    icon_t m_indicator;
  };

  using progressbar_t = shared_ptr<progressbar>;

  progressbar_t load_progressbar(
      const bar_settings& bar, const config& conf, string section, string name);
}

LEMONBUDDY_NS_END
