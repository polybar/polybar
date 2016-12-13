#pragma once

#include <i3ipc++/ipc.hpp>

#include "common.hpp"
#include "x11/randr.hpp"

POLYBAR_NS

class connection;

namespace i3_util {
  using connection_t = i3ipc::connection;
  using workspace_t = i3ipc::workspace_t;

  shared_ptr<workspace_t> focused_workspace(const connection_t&);

  vector<xcb_window_t> root_windows(connection& conn, const string& output_name = "");
  bool restack_above_root(connection& conn, const monitor_t& mon, const xcb_window_t win);
}

POLYBAR_NS_END
