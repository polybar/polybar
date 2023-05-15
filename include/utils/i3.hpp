#pragma once

#include <i3ipc++/ipc.hpp>

#include "common.hpp"
#include "utils/restack.hpp"
#include "x11/extensions/randr.hpp"

POLYBAR_NS

class connection;

namespace i3_util {
  using connection_t = i3ipc::connection;
  using workspace_t = i3ipc::workspace_t;

  const auto ws_numsort = [](shared_ptr<workspace_t> a, shared_ptr<workspace_t> b) { return a->num < b->num; };

  vector<shared_ptr<workspace_t>> workspaces(const connection_t& conn, const string& output = "", const bool show_urgent = false);
  shared_ptr<workspace_t> focused_workspace(const connection_t&);

  vector<xcb_window_t> root_windows(connection& conn, const string& output_name = "");
  restack_util::params get_restack_params(connection& conn);
}

namespace {
  inline bool operator==(i3_util::workspace_t& a, i3_util::workspace_t& b) {
    return a.num == b.num && a.output == b.output;
  }
  inline bool operator!=(i3_util::workspace_t& a, i3_util::workspace_t& b) {
    return !(a == b);
  }
}

POLYBAR_NS_END
