#pragma once

#include "modules/script.hpp"

POLYBAR_NS

namespace modules {
  class cmdscript_module : virtual public script_module {
   public:
    explicit cmdscript_module(const bar_settings&, string);

   protected:
    void process();
    chrono::duration<double> sleep_duration();
  };
}

POLYBAR_NS_END
