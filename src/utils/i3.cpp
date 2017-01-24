#include <xcb/xcb.h>
#include <i3ipc++/ipc.hpp>

#include "common.hpp"
#include "settings.hpp"
#include "utils/i3.hpp"
#include "utils/socket.hpp"
#include "utils/string.hpp"
#include "x11/connection.hpp"
#include "x11/ewmh.hpp"
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
   * Get main root window
   */
  xcb_window_t root_window(connection& conn) {
    auto children = conn.query_tree(conn.screen()->root).children();
    const auto wm_name = [&](xcb_connection_t* conn, xcb_window_t win) -> string {
      string title;
      if (!(title = ewmh_util::get_wm_name(win)).empty()) {
        return title;
      } else if (!(title = icccm_util::get_wm_name(conn, win)).empty()) {
        return title;
      } else {
        return "";
      }
    };

    for (auto it = children.begin(); it != children.end(); it++) {
      if (wm_name(conn, *it) == "i3") {
        return *it;
      }
    }

    return XCB_NONE;
  }

  /**
   * Restack given window relative to the i3 root window
   * defined for the given monitor
   *
   * Fixes the issue with always-on-top window's
   */
  bool restack_to_root(connection& conn, const xcb_window_t win) {
    const unsigned int value_mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
    const unsigned int value_list[2]{root_window(conn), XCB_STACK_MODE_ABOVE};
    if (value_list[0] != XCB_NONE) {
      conn.configure_window_checked(win, value_mask, value_list);
      return true;
    }
    return false;
  }
}

POLYBAR_NS_END
