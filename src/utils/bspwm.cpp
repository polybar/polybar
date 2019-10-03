#include <sys/un.h>

#include "errors.hpp"
#include "utils/bspwm.hpp"
#include "utils/env.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

namespace bspwm_util {
  /**
   * Get all bspwm root windows
   */
  vector<xcb_window_t> root_windows(connection& conn) {
    vector<xcb_window_t> roots;
    auto children = conn.query_tree(conn.screen()->root).children();

    for (auto it = children.begin(); it != children.end(); it++) {
      xcb_icccm_get_wm_class_reply_t reply{};
      reply.class_name = reply.instance_name = nullptr;

      if (xcb_icccm_get_wm_class_reply(conn, xcb_icccm_get_wm_class(conn, *it), &reply, nullptr)) {
        if (string_util::compare("Bspwm", reply.class_name) && string_util::compare("root", reply.instance_name)) {
          roots.emplace_back(*it);
        }
      }

      if (reply.class_name != nullptr || reply.instance_name != nullptr) {
        xcb_icccm_get_wm_class_reply_wipe(&reply);
      }
    }

    return roots;
  }

  /**
   * Restack given window relative to the bspwm root window
   * for the given monitor.
   *
   * Fixes the issue with always-on-top window's
   */
  bool restack_to_root(connection& conn, const monitor_t& mon, const xcb_window_t win) {
    for (auto&& root : root_windows(conn)) {
      auto geom = conn.get_geometry(root);

      if (mon->x != geom->x || mon->y != geom->y) {
        continue;
      }
      if (mon->w != geom->width || mon->h != geom->height) {
        continue;
      }

      const unsigned int value_mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
      const unsigned int value_list[2]{root, XCB_STACK_MODE_ABOVE};

      conn.configure_window_checked(win, value_mask, value_list);
      conn.flush();

      return true;
    }

    return false;
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
   * \code cpp
   *   auto ipc = make_connection();
   *   ipc->send(make_payload("desktop -f eDP-1:^1"));
   * \endcode
   */
  connection_t make_connection() {
    return socket_util::make_unix_connection(get_socket_path());
  }

  /**
   * Create a connection and subscribe to events
   * on the bspwm socket
   *
   * Example usage:
   * \code cpp
   *   auto ipc = make_subscriber();
   *
   *   while (!ipc->poll(POLLHUP, 0)) {
   *     ssize_t bytes_received = 0;
   *     auto data = ipc->receive(BUFSIZ-1, bytes_received, 0);
   *     std::cout << data << std::endl;
   *   }
   * \endcode
   */
  connection_t make_subscriber() {
    auto conn = make_connection();
    auto payload = make_payload("subscribe report");
    if (conn->send(payload->data, payload->len, 0) == 0) {
      throw system_error("Failed to initialize subscriber");
    }
    return conn;
  }
}

POLYBAR_NS_END
