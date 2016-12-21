#pragma once

#include "modules/meta/static_module.hpp"

POLYBAR_NS

namespace modules {
  class text_module : public static_module<text_module> {
   public:
    explicit text_module(const bar_settings&, string);

    string get_format() const;
    string get_output();
  };
}

POLYBAR_NS_END
