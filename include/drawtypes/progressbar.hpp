#pragma once

#include "common.hpp"
#include "components/builder.hpp"
#include "components/config.hpp"
#include "components/types.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace drawtypes {
  class progressbar : public non_copyable_mixin {
   public:
    explicit progressbar(const bar_settings& bar, int width, string format);

    void set_fill(label_t&& fill);
    void set_empty(label_t&& empty);
    void set_indicator(label_t&& indicator);
    void set_gradient(bool mode);
    void set_colors(vector<rgba>&& colors);

    string output(float percentage);

   protected:
    void fill(unsigned int perc, unsigned int fill_width);

   private:
    unique_ptr<builder> m_builder;
    vector<rgba> m_colors;
    string m_format;
    unsigned int m_width;
    unsigned int m_colorstep = 1;
    bool m_gradient = false;

    label_t m_fill;
    label_t m_empty;
    label_t m_indicator;
  };

  using progressbar_t = shared_ptr<progressbar>;

  progressbar_t load_progressbar(const bar_settings& bar, const config& conf, const string& section, string name);
}  // namespace drawtypes

POLYBAR_NS_END
