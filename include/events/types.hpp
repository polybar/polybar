#pragma once

#include <cstdint>

#include "common.hpp"

POLYBAR_NS

enum class event_type {
  NONE = 0,
  UPDATE,
  QUIT,
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
   * Create QUIT event
   */
  inline event make_none_evt() {
    return event{static_cast<int>(event_type::NONE)};
  }

  /**
   * Create QUIT event
   */
  inline event make_quit_evt(bool reload = false) {
    return event{static_cast<int>(event_type::QUIT), reload};
  }

  /**
   * Create UPDATE event
   */
  inline event make_update_evt(bool force = false) {
    return event{static_cast<int>(event_type::UPDATE), force};
  }
}

POLYBAR_NS_END
