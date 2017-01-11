#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <iomanip>

#include "errors.hpp"
#include "utils/file.hpp"
#include "utils/io.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace io_util {
  string read(int read_fd, size_t bytes_to_read) {
    fd_stream<std::istream> in(read_fd, false);
    char buffer[BUFSIZ];
    in.getline(buffer, bytes_to_read);
    string out{buffer};
    return out;
  }

  string readline(int read_fd) {
    fd_stream<std::istream> in(read_fd, false);
    string out;
    std::getline(in, out);
    return out;
  }

  size_t write(int write_fd, size_t bytes_to_write, const string& data) {
    fd_stream<std::ostream> out(write_fd, false);
    out.write(data.c_str(), bytes_to_write).flush();
    return out.good() ? data.size() : 0;
  }

  size_t writeline(int write_fd, const string& data) {
    fd_stream<std::ostream> out(write_fd, false);
    if (data[data.size() - 1] != '\n') {
      out << data << std::endl;
    } else {
      out << data << std::flush;
    }
    return out.good() ? data.size() : 0;
  }

  void tail(int read_fd, const function<void(string)>& callback) {
    string line;
    fd_stream<std::istream> in(read_fd, false);
    while (std::getline(in, line)) {
      callback(move(line));
    }
  }

  void tail(int read_fd, int writeback_fd) {
    tail(read_fd, [&](string data) { io_util::writeline(writeback_fd, data); });
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

  bool poll_write(int fd, int timeout_ms) {
    return poll(fd, POLLOUT, timeout_ms);
  }

  bool interrupt_read(int write_fd) {
    return write(write_fd, 1, {'\n'}) > 0;
  }

  void set_block(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
      throw system_error("Failed to unset O_NONBLOCK");
    }
  }

  void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
      throw system_error("Failed to set O_NONBLOCK");
    }
  }
}

POLYBAR_NS_END
