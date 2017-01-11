#pragma once

#include "common.hpp"

POLYBAR_NS

namespace io_util {
  string read(int read_fd, size_t bytes_to_read);
  string readline(int read_fd);

  size_t write(int write_fd, size_t bytes_to_write, const string& data);
  size_t writeline(int write_fd, const string& data);

  void tail(int read_fd, const function<void(string)>& callback);
  void tail(int read_fd, int writeback_fd);

  bool poll(int fd, short int events, int timeout_ms = 0);
  bool poll_read(int fd, int timeout_ms = 0);
  bool poll_write(int fd, int timeout_ms = 0);

  bool interrupt_read(int write_fd);

  void set_block(int fd);
  void set_nonblock(int fd);
}

POLYBAR_NS_END
