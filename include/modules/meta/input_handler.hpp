#pragma once

#include "common.hpp"

POLYBAR_NS

namespace modules {
  class input_handler {
   public:
    virtual ~input_handler() {}
    virtual bool input(string&& cmd) = 0;
  };
}

POLYBAR_NS_END
