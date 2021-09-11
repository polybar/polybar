#pragma once

#include <cstdint>

#include "common.hpp"

POLYBAR_NS

enum class event_type {
  NONE = 0,
  UPDATE,
};

struct event {
  int type{0};
  bool flag{false};
};

namespace {
  inline bool operator==(int id, event_type type) {
    return id == static_cast<int>(type);
  }
  inline bool operator!=(int id, event_type type) {
    return !(id == static_cast<int>(type));
  }

  /**
   * Create NONE event
   */
  inline event make_none_evt() {
    return event{static_cast<int>(event_type::NONE)};
  }

  /**
   * Create UPDATE event
   */
  inline event make_update_evt(bool force = false) {
    return event{static_cast<int>(event_type::UPDATE), force};
  }
}

POLYBAR_NS_END
