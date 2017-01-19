#include "x11/xembed.hpp"
#include "errors.hpp"
#include "x11/atoms.hpp"

POLYBAR_NS

namespace xembed {
  /**
   * Query _XEMBED_INFO for the given window
   */
  xembed_data* query(connection& conn, xcb_window_t win, xembed_data* data) {
    auto info = conn.get_property(false, win, _XEMBED_INFO, XCB_GET_PROPERTY_TYPE_ANY, 0L, 2);

    if (info->value_len == 0) {
      throw application_error("Invalid _XEMBED_INFO for window " + conn.id(win));
    }

    std::vector<unsigned int> xembed_data{info.value<unsigned int>().begin(), info.value<unsigned int>().end()};

    data->xembed = _XEMBED;
    data->xembed_info = _XEMBED_INFO;

    data->time = XCB_CURRENT_TIME;
    data->flags = xembed_data[1];
    data->version = xembed_data[0];

    return data;
  }

  /**
   * Send _XEMBED messages
   */
  void send_message(connection& conn, xcb_window_t target, long message, long d1, long d2, long d3) {
    auto msg = conn.make_client_message(_XEMBED, target);
    msg->data.data32[0] = XCB_CURRENT_TIME;
    msg->data.data32[1] = message;
    msg->data.data32[2] = d1;
    msg->data.data32[3] = d2;
    msg->data.data32[4] = d3;
    conn.send_client_message(msg, target);
  }

  /**
   * Send window focus event
   */
  void send_focus_event(connection& conn, xcb_window_t target) {
    auto msg = conn.make_client_message(WM_PROTOCOLS, target);
    msg->data.data32[0] = WM_TAKE_FOCUS;
    msg->data.data32[1] = XCB_CURRENT_TIME;
    conn.send_client_message(msg, target);
  }

  /**
   * Acknowledge window embedding
   */
  void notify_embedded(connection& conn, xcb_window_t win, xcb_window_t embedder, long version) {
    send_message(conn, win, XEMBED_EMBEDDED_NOTIFY, 0, embedder, version);
  }

  /**
   * Send window activate notification
   */
  void notify_activated(connection& conn, xcb_window_t win) {
    send_message(conn, win, XEMBED_WINDOW_ACTIVATE, 0, 0, 0);
  }

  /**
   * Send window deactivate notification
   */
  void notify_deactivated(connection& conn, xcb_window_t win) {
    send_message(conn, win, XEMBED_WINDOW_DEACTIVATE, 0, 0, 0);
  }

  /**
   * Send window focused notification
   */
  void notify_focused(connection& conn, xcb_window_t win, long focus_type) {
    send_message(conn, win, XEMBED_FOCUS_IN, focus_type, 0, 0);
  }

  /**
   * Send window unfocused notification
   */
  void notify_unfocused(connection& conn, xcb_window_t win) {
    send_message(conn, win, XEMBED_FOCUS_OUT, 0, 0, 0);
  }

  /**
   * Unembed given window
   */
  void unembed(connection& conn, xcb_window_t win, xcb_window_t root) {
    try {
      conn.unmap_window_checked(win);
      conn.reparent_window_checked(win, root, 0, 0);
    } catch (const xpp::x::error::window& err) {
      // invalid window
    }
  }
}

POLYBAR_NS_END
