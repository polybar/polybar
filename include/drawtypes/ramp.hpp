#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/labellist.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace drawtypes {
  class ramp : public labellist {
   public:
    explicit ramp() = default;
    explicit ramp(vector<label_t>&& icons) : labellist(move(icons)) {}

    void add(label_t&& icon);
    label_t get(size_t index);
    label_t get_by_percentage(float percentage);
    label_t get_by_percentage_with_borders(float percentage, float min, float max);
    label_t get_by_percentage_with_borders(int percentage, int min, int max);
    operator bool();

   protected:
  };

  using ramp_t = shared_ptr<ramp>;

  ramp_t load_ramp(const config& conf, const string& section, string name, bool required = true);
}

POLYBAR_NS_END
