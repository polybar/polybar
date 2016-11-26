#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <i3ipc++/ipc.hpp>

#include "common.hpp"
#include "config.hpp"
#include "utils/i3.hpp"
#include "utils/socket.hpp"
#include "utils/string.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

namespace i3_util {
  /**
   * Get all i3 root windows
   */
  vector<xcb_window_t> root_windows(connection& conn, const string& output_name) {
    vector<xcb_window_t> roots;
    auto children = conn.query_tree(conn.screen()->root).children();

    for (auto it = children.begin(); it != children.end(); it++) {
      xcb_icccm_get_text_property_reply_t reply;
      reply.name = nullptr;

      if (xcb_icccm_get_wm_name_reply(conn, xcb_icccm_get_wm_name(conn, *it), &reply, nullptr)) {
        if (("[i3 con] output " + output_name).compare(0, 16 + output_name.length(), reply.name) == 0) {
          roots.emplace_back(*it);
        }
      }

      if (reply.name != nullptr) {
        xcb_icccm_get_text_property_reply_wipe(&reply);
      }
    }

    return roots;
  }

  /**
   * Restack given window above the i3 root window
   * defined for the given monitor
   *
   * Fixes the issue with always-on-top window's
   */
  bool restack_above_root(connection& conn, const monitor_t& mon, const xcb_window_t win) {
    for (auto&& root : root_windows(conn, mon->name)) {
      const uint32_t value_mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
      const uint32_t value_list[2]{root, XCB_STACK_MODE_ABOVE};

      conn.configure_window_checked(win, value_mask, value_list);
      conn.flush();

      return true;
    }

    return false;
  }
}

POLYBAR_NS_END
