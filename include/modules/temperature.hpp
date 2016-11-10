#pragma once

#include <istream>

#include "config.hpp"
#include "drawtypes/label.hpp"
#include "modules/meta.hpp"
#include "utils/file.hpp"

LEMONBUDDY_NS

namespace modules {
  enum class temp_state { NORMAL = 0, WARN };


  class temperature_module : public timer_module<temperature_module> {
   public:
    using timer_module::timer_module;

    void setup();
    bool update();
    string get_format() const;
    bool build(builder* builder, string tag) const;

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_LABEL_WARN = "<label-warn>";
    static constexpr auto FORMAT_WARN = "format-warn";

    map<temp_state, label_t> m_label;

    string m_path;

    int m_zone;
    int m_tempwarn;
    int m_temp = 0;
  };
}

LEMONBUDDY_NS_END
