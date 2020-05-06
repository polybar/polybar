#pragma once

#include "common.hpp"

POLYBAR_NS

namespace modules {
  class input_handler {
   public:
    virtual ~input_handler() {}
    /**
     * Handle action
     *
     * \returns true if the action is supported and false otherwise
     */
    virtual bool input(string&& action) = 0;

    /**
     * The name of this input handler
     *
     * Actions of the form '#NAME#ACTION' can be sent to this handler if NAME is the name of this input handler.
     */
    virtual string input_handler_name() const = 0;
  };
}

POLYBAR_NS_END
