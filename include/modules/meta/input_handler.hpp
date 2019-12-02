#pragma once

#include "common.hpp"

POLYBAR_NS

namespace modules {
  class input_handler {
   public:
    virtual ~input_handler() {}
    /**
     * Handle command
     *
     * \returns true if the command is supported and false otherwise
     */
    virtual bool input(string&& cmd) = 0;

    /**
     * The name of this input handler
     *
     * Actions of the form '#NAME#ACTION' can be sent to this handler if NAME is the name of this input handler.
     */
    virtual string input_handler_name() const = 0;
  };
}

POLYBAR_NS_END
