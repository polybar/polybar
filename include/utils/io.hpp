#pragma once

#include "common.hpp"

POLYBAR_NS

namespace io_util {
  void tail(int read_fd, const function<void(string)>& callback);

  bool poll(int fd, short int events, int timeout_ms = 0);
  bool poll_read(int fd, int timeout_ms = 0);
}  // namespace io_util

POLYBAR_NS_END
