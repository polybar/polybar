#pragma once

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

#include "common.hpp"
#include "config.hpp"
#include "utils/socket.hpp"
#include "utils/string.hpp"
#include "x11/connection.hpp"
#include "x11/randr.hpp"
#include "x11/window.hpp"

LEMONBUDDY_NS

namespace bspwm_util {
  struct payload;
  using connection_t = unique_ptr<socket_util::unix_connection>;
  using payload_t = unique_ptr<payload>;

  struct payload {
    char data[BUFSIZ]{'\0'};
    size_t len = 0;
  };

  vector<xcb_window_t> root_windows(connection& conn);
  bool restack_above_root(connection& conn, const monitor_t& mon, const xcb_window_t win);

  string get_socket_path();

  payload_t make_payload(string cmd);
  connection_t make_connection();
  connection_t make_subscriber();
}

LEMONBUDDY_NS_END
