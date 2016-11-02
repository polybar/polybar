#pragma once

#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  class text_module : public static_module<text_module> {
   public:
    using static_module::static_module;

    void setup();
    string get_format() const;
    string get_output();
  };
}

LEMONBUDDY_NS_END
