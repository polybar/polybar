#pragma once

#include <i3ipc++/ipc.hpp>

#include "common.hpp"
#include "x11/connection.hpp"
#include "x11/randr.hpp"

POLYBAR_NS

namespace i3_util {
  using connection_t = i3ipc::connection;

  vector<xcb_window_t> root_windows(connection& conn, const string& output_name = "");
  bool restack_above_root(connection& conn, const monitor_t& mon, const xcb_window_t win);
}

POLYBAR_NS_END
