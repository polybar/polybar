#pragma once

#include <xcb/xcb.h>

#include "common.hpp"
#include "config.hpp"
#include "utils/socket.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

namespace bspwm_util {
  struct payload;
  using connection_t = unique_ptr<socket_util::unix_connection>;
  using payload_t = unique_ptr<payload>;

  /**
   * bspwm payload
   */
  struct payload {
    char data[BUFSIZ]{'\0'};
    size_t len = 0;
  };

  /**
   * Get path to the bspwm socket by the following order
   *
   * 1. Value of environment variable BSPWM_SOCKET
   * 2. Value built from the bspwm socket path template
   * 3. Value of the macro BSPWM_SOCKET_PATH
   */
  string get_socket_path() {
    string env_path;

    if ((env_path = read_env("BSPWM_SOCKET")).empty() == false)
      return env_path;

    struct sockaddr_un sa;
    char* host = nullptr;
    int dsp = 0;
    int scr = 0;

    if (xcb_parse_display(nullptr, &host, &dsp, &scr) == 0)
      return BSPWM_SOCKET_PATH;

    snprintf(sa.sun_path, sizeof(sa.sun_path), "/tmp/bspwm%s_%i_%i-socket", host, dsp, scr);

    return sa.sun_path;
  }

  /**
   * Generate a payload object with properly formatted data
   * ready to be sent to the bspwm ipc controller
   */
  unique_ptr<payload> make_payload(string cmd) {
    auto pl = make_unique<payload>();
    auto size = sizeof(pl->data);
    int offset = 0;
    int chars = 0;

    for (auto&& word : string_util::split(cmd, ' ')) {
      chars = snprintf(pl->data + offset, size - offset, "%s%c", word.c_str(), 0);
      pl->len += chars;
      offset += chars;
    }

    return pl;
  }

  /**
   * Create an ipc socket connection
   *
   * Example usage:
   * @code cpp
   *   auto ipc = bspwm_util::make_connection();
   *   ipc->send(bspwm_util::make_payload("desktop -f eDP-1:^1"));
   * @endcode
   */
  connection_t make_connection() {
    return socket_util::make_unix_connection(get_socket_path());
  }

  /**
   * Create a connection and subscribe to events
   * on the bspwm socket
   *
   * Example usage:
   * @code cpp
   *   auto ipc = bspwm_util::make_subscriber();
   *
   *   while (!ipc->poll(POLLHUP, 0)) {
   *     ssize_t bytes_received = 0;
   *     auto data = ipc->receive(BUFSIZ-1, bytes_received, 0);
   *     std::cout << data << std::endl;
   *   }
   * @endcode
   */
  connection_t make_subscriber() {
    auto conn = make_connection();
    auto payload = make_payload("subscribe report");
    if (conn->send(payload->data, payload->len, 0) == 0)
      throw system_error("Failed to initialize subscriber");
    return conn;
  }
}

LEMONBUDDY_NS_END
