#pragma once

#include "modules/meta.hpp"

POLYBAR_NS

namespace modules {
  class text_module : public static_module<text_module> {
   public:
    using static_module::static_module;

    void setup();
    string get_format() const;
    string get_output();
  };
}

POLYBAR_NS_END
