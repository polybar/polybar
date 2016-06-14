#include <fstream>
#include <string>
#include <sstream>
#include <string.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "services/logger.hpp"
#include "utils/io.hpp"
#include "utils/proc.hpp"
#include "utils/string.hpp"
#include "utils/macros.hpp"

namespace io
{
  namespace socket
  {
    int open(const std::string& path)
    {
      int fd;
      struct sockaddr_un sock_addr;

      if ((fd = ::socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        log_error("[io::socket::open] Error opening socket: "+ StrErrno());
        return -1;
      }

      sock_addr.sun_family = AF_UNIX;
      std::snprintf(sock_addr.sun_path, sizeof(sock_addr.sun_path), "%s", path.c_str());

      if (connect(fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1) {
        log_error("[io::socket::open] Error connecting to socket: "+ StrErrno());
        return -1;
      }

      return fd;
    }

    int send(int fd, const std::string& data, int flags)
    {
      int bytes = ::send(fd, data.c_str(), data.size()+1, flags);
      if (bytes == -1)
        log_error("[io::socket::send] Error sending data: "+ StrErrno());
      return bytes;
    }

    int recv(int fd, char *buffer, int recv_bytes, int flags)
    {
      int bytes = ::recv(fd, buffer, recv_bytes, flags);
      if (bytes > 0)
        buffer[bytes] = 0;
      else if (bytes == -1)
        log_error("[io::socket::recv] Error receiving data: "+ StrErrno());
      return bytes;
    }
  }

  namespace file
  {
    bool exists(const std::string& fname)
    {
      struct stat buffer;
      return (stat(fname.c_str(), &buffer) == 0);
    }

    std::string get_contents(const std::string& fname)
    {
      try {
        std::ifstream ifs(fname);
        std::string contents(
            (std::istreambuf_iterator<char>(ifs)),
            (std::istreambuf_iterator<char>()));
        return contents;
      } catch (std::ios_base::failure &e) {
        log_error(e.what());
        return "";
      }
    }

    bool is_fifo(const std::string& fname)
    {
      FILE *fp = fopen(fname.c_str(), "r");
      int fd = fileno(fp);
      struct stat statbuf;
      fstat(fd, &statbuf);
      bool is_fifo = S_ISFIFO(statbuf.st_mode);
      fclose(fp);
      return is_fifo;
    }

    std::size_t write(io::file::FilePtr *fptr, const std::string& data) {
      auto buf = data.c_str();
      return fwrite(buf, sizeof(char), sizeof(buf), (*fptr)());
    }

    std::size_t write(const std::string& fpath, const std::string& data) {
      return io::file::write(std::make_unique<FilePtr>(fpath, "a+").get(), data);
    }
  }

  std::string read(int read_fd, int bytes_to_read, int &bytes_read_loc, int &status_loc)
  {
    char buffer[BUFSIZ-1];

    if (bytes_to_read == -1)
      bytes_to_read = sizeof(buffer);

    status_loc = 0;

    if ((bytes_read_loc = ::read(read_fd, &buffer, bytes_to_read)) > 0) {
      // buffer[bytes_read_loc] = 0;
    } else if (bytes_read_loc == 0) {
      get_logger()->debug("Reached EOF");
      status_loc = -1;
    } else if (bytes_read_loc == -1) {
      get_logger()->debug("Read failed");
      status_loc = errno;
    }

    return "";
  }

  std::string read(int read_fd, int bytes_to_read)
  {
    int bytes_read = 0;
    int status = 0;
    return read(read_fd, bytes_to_read, bytes_read, status);
  }

  std::string readline(int read_fd, int &bytes_read)
  {
    std::stringstream buffer;
    char c;

    while ((bytes_read = ::read(read_fd, &c, 1)) > 0) {
      buffer << c;
      if (c == '\n' || c == '\x00') break;
    }

    if (bytes_read == 0) {
      get_logger()->debug("Reached EOF");
    } else if (bytes_read == -1) {
      get_logger()->debug("Read failed");
    } else {
      return string::strip_trailing_newline(buffer.str());
    }

    return "";
  }

  std::string readline(int read_fd)
  {
    int bytes_read;
    return readline(read_fd, bytes_read);
  }

  int write(int write_fd, const std::string& data) {
    return ::write(write_fd, data.c_str(), strlen(data.c_str()));
  }

  int writeline(int write_fd, const std::string& data)
  {
    if (data.length() == 0) return -1;
    if (data.substr(data.length()-1, 1) != "\n")
      return io::write(write_fd, data+"\n");
    else
      return io::write(write_fd, data);
  }

  void tail(int read_fd, std::function<void(std::string)> callback)
  {
    int bytes_read;
    while (true) {
      auto line = io::readline(read_fd, bytes_read);
      if (bytes_read <= 0)
        break;
      callback(line);
    }
  }

  void tail(int read_fd, int writeback_fd)
  {
    tail(read_fd, [&writeback_fd](std::string data){
      io::writeline(writeback_fd, data);
    });
  }

  bool poll_read(int fd, int timeout_ms) {
    return poll(fd, POLLIN, timeout_ms);
  }

  // bool poll_write(int fd, int timeout_ms) {
  //   return poll(fd, POLLOUT, timeout_ms);
  // }

  bool poll(int fd, short int events, int timeout_ms)
  {
    // timeout_ms = -1 disables the timeout

    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = events;

    // 1 = nfds_t (n)umber of (f)ile (d)escriptors
    ::poll(fds, 1, timeout_ms);

    return fds[0].revents & events;
  }

  // int get_flags(int fd)
  // {
  //   int flags;
  //   if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
  //     return 0;
  //   return flags;
  // }

  // int set_blocking(int fd) {
  //   return fcntl(fd, F_SETFL, io::get_flags(fd) & ~O_NONBLOCK);
  // }

  // int set_non_blocking(int fd) {
  //   return fcntl(fd, F_SETFL, io::get_flags(fd) | O_NONBLOCK);
  // }
}
