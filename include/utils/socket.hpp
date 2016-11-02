#pragma once

#include <sys/poll.h>

#include "common.hpp"

LEMONBUDDY_NS

namespace socket_util {
  class unix_connection {
   public:
    explicit unix_connection(string&& path);

    ~unix_connection() noexcept;

    int disconnect();

    ssize_t send(const void* data, size_t len, int flags = 0);
    ssize_t send(string data, int flags = 0);

    string receive(const ssize_t receive_bytes, ssize_t& bytes_received_addr, int flags = 0);
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
  auto make_unix_connection = [](string&& path) -> unique_ptr<unix_connection> {
    return make_unique<unix_connection>(forward<string>(path));
  };
}

LEMONBUDDY_NS_END
