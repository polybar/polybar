#pragma once

#include <istream>

#include "modules/meta/timer_module.hpp"
#include "modules/meta/types.hpp"
#include "settings.hpp"

POLYBAR_NS

namespace modules {
  enum class temp_state { NORMAL = 0, WARN };

  class temperature_module : public timer_module<temperature_module> {
   public:
    explicit temperature_module(const bar_settings&, string, const config&);

    bool update();
    string get_format() const;
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = TEMPERATURE_TYPE;

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_LABEL_WARN = "<label-warn>";
    static constexpr auto TAG_RAMP = "<ramp>";
    static constexpr auto FORMAT_WARN = "format-warn";

    map<temp_state, label_t> m_label;
    ramp_t m_ramp;

    string m_path;
    string m_zone_type;
    int m_zone = 0;
    // Base temperature used for where to start the ramp
    int m_tempbase = 0;
    int m_tempwarn = 0;
    int m_temp = 0;

    // Whether or not to show units with the %temperature-X% tokens
    bool m_units{true};
  };
}  // namespace modules

POLYBAR_NS_END
