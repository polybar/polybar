#pragma once

#include "common.hpp"

LEMONBUDDY_NS

namespace io_util {
  string read(int read_fd, int bytes_to_read, int& bytes_read_loc, int& status_loc);
  string read(int read_fd, int bytes_to_read = -1);
  string readline(int read_fd, int& bytes_read);
  string readline(int read_fd);

  size_t write(int write_fd, string data);
  size_t writeline(int write_fd, string data);

  void tail(int read_fd, function<void(string)> callback);
  void tail(int read_fd, int writeback_fd);

  bool poll(int fd, short int events, int timeout_ms = 1);
  bool poll_read(int fd, int timeout_ms = 1);
  bool poll_write(int fd, int timeout_ms = 1);

  void interrupt_read(int write_fd);
}

LEMONBUDDY_NS_END
