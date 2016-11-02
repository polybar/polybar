
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "utils/file.hpp"
#include "utils/mixins.hpp"
#include "utils/socket.hpp"

LEMONBUDDY_NS

using std::snprintf;
using std::strlen;

namespace socket_util {
  /**
   * Constructor: establishing socket connection
   */
  unix_connection::unix_connection(string&& path) : m_socketpath(path) {
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
  unix_connection::~unix_connection() noexcept {
    if (m_fd != -1)
      close(m_fd);
  }

  /**
   * Close reading end of connection
   */
  int unix_connection::disconnect() {
    return shutdown(m_fd, SHUT_RD);
  }

  /**
   * Transmit fixed size data
   */
  ssize_t unix_connection::send(const void* data, size_t len, int flags) {
    ssize_t bytes_sent = 0;

    if ((bytes_sent = ::send(m_fd, data, len, flags)) == -1)
      throw system_error("Failed to transmit data");

    return bytes_sent;
  }

  /**
   * Transmit string data
   */
  ssize_t unix_connection::send(string data, int flags) {
    return send(data.c_str(), data.length(), flags);
  }

  /**
   * Receive data
   */
  string unix_connection::receive(
      const ssize_t receive_bytes, ssize_t& bytes_received_addr, int flags) {
    char buffer[BUFSIZ];

    bytes_received_addr = ::recv(m_fd, buffer, receive_bytes, flags);
    if (bytes_received_addr == -1)
      throw system_error("Failed to receive data");

    buffer[bytes_received_addr] = 0;
    return string{buffer};
  }

  /**
   * Poll file descriptor for to check for available data
   */
  bool unix_connection::poll(short int events, int timeout_ms) {
    struct pollfd fds[1];
    fds[0].fd = m_fd;
    fds[0].events = events;

    if (::poll(fds, 1, timeout_ms) == -1)
      throw system_error("Failed to poll file descriptor");

    return fds[0].revents & events;
  }
}

LEMONBUDDY_NS_END
