#pragma once

#include "common.hpp"
#include "events/signal.hpp"
#include "events/signal_receiver.hpp"

POLYBAR_NS

namespace modules {
  using input_event_t = signals::eventqueue::process_input;
  class input_handler : public signal_receiver<0, input_event_t> {
   public:
    virtual ~input_handler() {}
  };
}

POLYBAR_NS_END
