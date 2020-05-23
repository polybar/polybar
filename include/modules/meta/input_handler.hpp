#pragma once

#include "common.hpp"

POLYBAR_NS

namespace modules {
  class input_handler {
   public:
    virtual ~input_handler() {}
    /**
     * Handle action, possibly with data attached
     *
     * Any implementation is free to ignore the data, if the action does not
     * require additional data.
     *
     * \returns true if the action is supported and false otherwise
     */
    virtual bool input(string&& action, string&& data) = 0;

    /**
     * The name of this input handler
     */
    virtual string input_handler_name() const = 0;
  };
}

POLYBAR_NS_END
