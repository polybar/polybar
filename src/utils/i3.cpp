#include <xcb/xcb.h>
#include <i3ipc++/ipc.hpp>

#include "common.hpp"
#include "config.hpp"
#include "utils/i3.hpp"
#include "utils/socket.hpp"
#include "utils/string.hpp"
#include "x11/connection.hpp"
#include "x11/icccm.hpp"

POLYBAR_NS

namespace i3_util {
  /**
   * Get all workspaces for given output
   */
  vector<shared_ptr<workspace_t>> workspaces(const connection_t& conn, const string& output) {
    vector<shared_ptr<workspace_t>> result;
    for (auto&& ws : conn.get_workspaces()) {
      if (output.empty() || ws->output == output) {
        result.emplace_back(forward<decltype(ws)>(ws));
      }
    }
    return result;
  }

  /**
   * Get currently focused workspace
   */
  shared_ptr<workspace_t> focused_workspace(const connection_t& conn) {
    for (auto&& ws : conn.get_workspaces()) {
      if (ws->focused) {
        return ws;
      }
    }
    return nullptr;
  }

  /**
   * Get all i3 root windows
   */
  vector<xcb_window_t> root_windows(connection& conn, const string& output_name) {
    vector<xcb_window_t> roots;
    auto children = conn.query_tree(conn.screen()->root).children();

    for (auto it = children.begin(); it != children.end(); it++) {
      auto wm_name = icccm_util::get_wm_name(conn, *it);
      if (wm_name.compare("[i3 con] output " + output_name) == 0) {
        roots.emplace_back(*it);
      }
    }

    return roots;
  }

  /**
   * Restack given window relative to the i3 root window
   * defined for the given monitor
   *
   * Fixes the issue with always-on-top window's
   */
  bool restack_to_root(connection& conn, const monitor_t& mon, const xcb_window_t win) {
    for (auto&& root : root_windows(conn, mon->name)) {
      const uint32_t value_mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
      const uint32_t value_list[2]{root, XCB_STACK_MODE_BELOW};
      conn.configure_window_checked(win, value_mask, value_list);
      return true;
    }

    return false;
  }
}

POLYBAR_NS_END
