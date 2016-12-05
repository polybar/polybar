#pragma once

#include "common.hpp"

POLYBAR_NS

enum class event_type;
struct event;
struct quit_event;
struct update_event;
struct input_event;

class eventloop {
 public:
  using entry_t = event;
};

POLYBAR_NS_END
