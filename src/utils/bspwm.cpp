#include "utils/bspwm.hpp"

#include <sys/un.h>
#include <xcb/xcb.h>

#include "errors.hpp"
#include "utils/env.hpp"
#include "utils/string.hpp"
#include "x11/connection.hpp"
#include "x11/icccm.hpp"

POLYBAR_NS

namespace bspwm_util {
/**
 * Returns window against which to restack.
 *
 * Bspwm creates one root window per monitor with a `WM_CLASS` value of `root\0Bspwm` and the window taking up the
 * entire monitor.
 * For overlapping monitors, stacking polybar above the root window for its monitor, but below the root window for an
 * overlapping monitor, may cause the upper root window to obstruct polybar, at least in terms of receiving mouse
 * clicks. Because of that, we simply restack polybar above the topmost root window.
 */
restack_util::params get_restack_params(connection& conn) {
  auto children = conn.query_tree(conn.root()).children();

  xcb_window_t top_root = XCB_NONE;

  // Iteration happens from bottom to top
  for (xcb_window_t wid : children) {
    auto [instance_name, class_name] = icccm_util::get_wm_class(conn, wid);

    if (string_util::compare("root", instance_name) && string_util::compare("Bspwm", class_name)) {
      top_root = wid;
    }
  }

  return {top_root, XCB_STACK_MODE_ABOVE};
}

/**
 * Get path to the bspwm socket by the following order
 *
 * 1. Value of environment variable BSPWM_SOCKET
 * 2. Value built from the bspwm socket path template
 * 3. Value of the macro BSPWM_SOCKET_PATH
 */
string get_socket_path() {
  string env_path;

  if (!(env_path = env_util::get("BSPWM_SOCKET")).empty()) {
    return env_path;
  }

  struct sockaddr_un sa {};
  char* host = nullptr;
  int dsp = 0;
  int scr = 0;

  if (xcb_parse_display(nullptr, &host, &dsp, &scr) == 0) {
    return BSPWM_SOCKET_PATH;
  }

  snprintf(sa.sun_path, sizeof(sa.sun_path), "/tmp/bspwm%s_%i_%i-socket", host, dsp, scr);
  free(host);

  return sa.sun_path;
}

/**
 * Generate a payload object with properly formatted data
 * ready to be sent to the bspwm ipc controller
 */
payload_t make_payload(const string& cmd) {
  payload_t payload{new payload_t::element_type{}};
  auto size = sizeof(payload->data);
  int offset = 0;
  int chars = 0;

  for (auto&& word : string_util::split(cmd, ' ')) {
    chars = snprintf(payload->data + offset, size - offset, "%s%c", word.c_str(), 0);
    payload->len += chars;
    offset += chars;
  }

  return payload;
}

/**
 * Create an ipc socket connection
 *
 * Example usage:
 * @code cpp
 *   auto ipc = make_connection();
 *   ipc->send(make_payload("desktop -f eDP-1:^1"));
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
 *   auto ipc = make_subscriber();
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
  if (conn->send(payload->data, payload->len, 0) == 0) {
    throw system_error("Failed to initialize subscriber");
  }
  return conn;
}
} // namespace bspwm_util

POLYBAR_NS_END
