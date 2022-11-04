#include "utils/io.hpp"

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <iomanip>

#include "errors.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace io_util {
  void tail(int read_fd, const function<void(string)>& callback) {
    string line;
    fd_stream<std::istream> in(read_fd, false);
    while (std::getline(in, line)) {
      callback(move(line));
    }
  }

  bool poll(int fd, short int events, int timeout_ms) {
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = events;
    ::poll(fds, 1, timeout_ms);
    return fds[0].revents & events;
  }

  bool poll_read(int fd, int timeout_ms) {
    return poll(fd, POLLIN, timeout_ms);
  }
}  // namespace io_util

POLYBAR_NS_END
