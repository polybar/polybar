#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "errors.hpp"
#include "utils/file.hpp"
#include "utils/mixins.hpp"
#include "utils/socket.hpp"

POLYBAR_NS

using std::snprintf;
using std::strlen;

namespace socket_util {
  /**
   * Constructor: establishing socket connection
   */
  unix_connection::unix_connection(string&& path) : m_socketpath(path) {
    struct sockaddr_un socket_addr {};
    socket_addr.sun_family = AF_UNIX;

    if ((m_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
      throw system_error("Failed to open unix connection");
    }

    snprintf(socket_addr.sun_path, sizeof(socket_addr.sun_path), "%s", m_socketpath.c_str());
    auto len = sizeof(socket_addr);

    if (connect(m_fd, reinterpret_cast<struct sockaddr*>(&socket_addr), len) == -1) {
      throw system_error("Failed to connect to socket");
    }
  }

  /**
   * Destructor: closes file descriptor
   */
  unix_connection::~unix_connection() noexcept {
    if (m_fd != -1) {
      close(m_fd);
    }
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

    if ((bytes_sent = ::send(m_fd, data, len, flags)) == -1) {
      throw system_error("Failed to transmit data");
    }

    return bytes_sent;
  }

  /**
   * Transmit string data
   */
  ssize_t unix_connection::send(const string& data, int flags) {
    return send(data.c_str(), data.length(), flags);
  }

  /**
   * Receive data
   */
  string unix_connection::receive(const ssize_t receive_bytes, ssize_t* bytes_received, int flags) {
    char buffer[BUFSIZ];

    if ((*bytes_received = ::recv(m_fd, buffer, receive_bytes, flags)) == -1) {
      throw system_error("Failed to receive data");
    } else {
      buffer[*bytes_received] = 0;
    }

    return string{buffer};
  }

  /**
   * @see receive
   */
  string unix_connection::receive(const ssize_t receive_bytes, int flags) {
    ssize_t bytes{0};
    return receive(receive_bytes, &bytes, flags);
  }

  /**
   * Peek at the specified number of bytes
   */
  bool unix_connection::peek(const size_t peek_bytes) {
    ssize_t bytes_seen{0};
    receive(peek_bytes, &bytes_seen, MSG_PEEK);
    return bytes_seen > 0;
  }

  /**
   * Poll file descriptor for to check for available data
   */
  bool unix_connection::poll(short int events, int timeout_ms) {
    struct pollfd fds[1];
    fds[0].fd = m_fd;
    fds[0].events = events;

    if (::poll(fds, 1, timeout_ms) == -1) {
      throw system_error("Failed to poll file descriptor");
    }

    return fds[0].revents & events;
  }
}

POLYBAR_NS_END
