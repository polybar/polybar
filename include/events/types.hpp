#pragma once

#include <cstdint>

enum class event_type : uint8_t {
  NONE = 0,
  UPDATE,
  CHECK,
  INPUT,
  QUIT,
};

struct event {
  uint8_t type{0};
  bool flag{false};
};

namespace {
  /**
   * Create QUIT event
   */
  inline event make_quit_evt(bool reload = false) {
    return event{static_cast<uint8_t>(event_type::QUIT), reload};
  }

  /**
   * Create UPDATE event
   */
  inline event make_update_evt(bool force = false) {
    return event{static_cast<uint8_t>(event_type::UPDATE), force};
  }

  /**
   * Create INPUT event
   */
  inline event make_input_evt() {
    return event{static_cast<uint8_t>(event_type::INPUT)};
  }

  /**
   * Create CHECK event
   */
  inline event make_check_evt() {
    return event{static_cast<uint8_t>(event_type::CHECK)};
  }
}
