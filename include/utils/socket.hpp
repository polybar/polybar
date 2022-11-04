#pragma once

#include <poll.h>

#include "common.hpp"

POLYBAR_NS

namespace socket_util {
  class unix_connection {
   public:
    explicit unix_connection(string&& path);

    ~unix_connection() noexcept;

    int disconnect();

    ssize_t send(const void* data, size_t len, int flags = 0);
    ssize_t send(const string& data, int flags = 0);

    string receive(const ssize_t receive_bytes, int flags = 0);
    string receive(const ssize_t receive_bytes, ssize_t* bytes_received, int flags = 0);

    bool peek(const size_t peek_bytes);
    bool poll(short int events = POLLIN, int timeout_ms = -1);

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
  inline unique_ptr<unix_connection> make_unix_connection(string&& path) {
    return std::make_unique<unix_connection>(forward<string>(path));
  }
}  // namespace socket_util

POLYBAR_NS_END
