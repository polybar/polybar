#pragma once

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "common.hpp"
#include "utils/file.hpp"
#include "utils/mixins.hpp"

LEMONBUDDY_NS

using std::snprintf;
using std::strlen;

namespace socket_util {
  class unix_connection {
   public:
    /**
     * Constructor: establishing socket connection
     */
    explicit unix_connection(string&& path) : m_socketpath(path) {
      struct sockaddr_un socket_addr;
      socket_addr.sun_family = AF_UNIX;

      if ((m_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        throw system_error("Failed to open unix connection");

      snprintf(socket_addr.sun_path, sizeof(socket_addr.sun_path), "%s", m_socketpath.c_str());
      auto len = sizeof(socket_addr);

      if (connect(m_fd, reinterpret_cast<struct sockaddr*>(&socket_addr), len) == -1)
        throw system_error("Failed to connect to socket");
    }

    /**
     * Destructor: closes file descriptor
     */
    ~unix_connection() noexcept {
      if (m_fd != -1)
        close(m_fd);
    }

    /**
     * Close reading end of connection
     */
    int disconnect() {
      return shutdown(m_fd, SHUT_RD);
    }

    /**
     * Transmit fixed size data
     */
    auto send(const void* data, size_t len, int flags = 0) {
      auto bytes_sent = 0;

      if ((bytes_sent = ::send(m_fd, data, len, flags)) == -1)
        throw system_error("Failed to transmit data");

      return bytes_sent;
    }

    /**
     * Transmit string data
     */
    auto send(string data, int flags = 0) {
      return send(data.c_str(), data.length(), flags);
    }

    /**
     * Receive data
     */
    auto receive(ssize_t receive_bytes, ssize_t& bytes_received_addr, int flags = 0) {
      char buffer[receive_bytes + 1];

      bytes_received_addr = ::recv(m_fd, buffer, receive_bytes, flags);
      if (bytes_received_addr == -1)
        throw system_error("Failed to receive data");

      buffer[bytes_received_addr] = 0;
      return string{buffer};
    }

    /**
     * Poll file descriptor for to check for available data
     */
    auto poll(short int events = POLLIN, int timeout_ms = -1) {
      struct pollfd fds[1];
      fds[0].fd = m_fd;
      fds[0].events = events;

      if (::poll(fds, 1, timeout_ms) == -1)
        throw system_error("Failed to poll file descriptor");

      return fds[0].revents & events;
    }

   protected:
    int m_fd = -1;
    string m_socketpath;
  };

  /**
   * Creates a wrapper for a unix socket connection
   *
   * Example usage:
   * @code cpp
   *   auto conn = socket_util::make_unix_connection("/tmp/socket");
   *   conn->send(...);
   *   conn->receive(...);
   * @endcode
   */
  auto make_unix_connection = [](string&& path) -> unique_ptr<unix_connection> {
    return make_unique<unix_connection>(forward<string>(path));
  };
}

LEMONBUDDY_NS_END
