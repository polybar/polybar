#pragma once

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

#include "common.hpp"
#include "settings.hpp"
#include "utils/restack.hpp"
#include "utils/socket.hpp"
#include "utils/string.hpp"
#include "x11/extensions/randr.hpp"
#include "x11/window.hpp"

POLYBAR_NS

class connection;

namespace bspwm_util {
  struct payload;
  using connection_t = unique_ptr<socket_util::unix_connection>;
  using payload_t = unique_ptr<payload>;

  struct payload {
    char data[BUFSIZ]{'\0'};
    size_t len = 0;
  };

  vector<xcb_window_t> root_windows(connection& conn);
  restack_util::params get_restack_params(connection& conn, const monitor_t& mon, xcb_window_t bar_window);

  string get_socket_path();

  payload_t make_payload(const string& cmd);
  connection_t make_connection();
  connection_t make_subscriber();
}

POLYBAR_NS_END
