#include "x11/xembed.hpp"

#include <cassert>

#include "errors.hpp"
#include "utils/string.hpp"
#include "x11/atoms.hpp"
#include "x11/ewmh.hpp"

POLYBAR_NS

namespace xembed {

void info::set(const uint32_t* data) {
  version = data[0];
  flags = data[1];
}

uint32_t info::get_version() const {
  return version;
}

uint32_t info::get_flags() const {
  return flags;
}

bool info::is_mapped() const {
  return (flags & XEMBED_MAPPED) == XEMBED_MAPPED;
}

string info::to_string() const {
  return sstream() << "xembed(version=0x" << std::hex << get_version() << ", flags=0x" << get_flags()
                   << ", XEMBED_MAPPED=" << std::dec << is_mapped() << ")";
}

/**
 * Query _XEMBED_INFO for the given window
 *
 * @return Whether valid XEMBED info was found
 */
bool query(connection& conn, xcb_window_t win, info& data) {
  auto info = conn.get_property(false, win, _XEMBED_INFO, _XEMBED_INFO, 0L, 2);

  if (info->value_len == 0) {
    return false;
  }

  std::vector<uint32_t> xembed_data{info.value<uint32_t>().begin(), info.value<uint32_t>().end()};
  data.set(xembed_data.data());

  return true;
}

/**
 * Send _XEMBED messages
 */
void send_message(connection& conn, xcb_window_t target, uint32_t message, uint32_t d1, uint32_t d2, uint32_t d3) {
  auto msg = conn.make_client_message(_XEMBED, target);
  msg.data.data32[0] = XCB_CURRENT_TIME;
  msg.data.data32[1] = message;
  msg.data.data32[2] = d1;
  msg.data.data32[3] = d2;
  msg.data.data32[4] = d3;
  conn.send_client_message(msg, target);
}

/**
 * Send window focus event
 */
void send_focus_event(connection& conn, xcb_window_t target) {
  auto msg = conn.make_client_message(WM_PROTOCOLS, target);
  msg.data.data32[0] = WM_TAKE_FOCUS;
  msg.data.data32[1] = XCB_CURRENT_TIME;
  conn.send_client_message(msg, target);
}

/**
 * Acknowledge window embedding
 */
void notify_embedded(connection& conn, xcb_window_t win, xcb_window_t embedder, uint32_t version) {
  send_message(conn, win, XEMBED_EMBEDDED_NOTIFY, 0, embedder, std::min(version, XEMBED_MAX_VERSION));
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
void notify_focused(connection& conn, xcb_window_t win, uint32_t focus_type) {
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
  assert(win != XCB_NONE);
  try {
    conn.unmap_window_checked(win);
    conn.reparent_window_checked(win, root, 0, 0);
  } catch (const xpp::x::error::window& err) {
    // invalid window
    logger::make().err("tray: Failed to unembed window '%s' (%s)", ewmh_util::get_wm_name(win), conn.id(win));
  }
}
} // namespace xembed

POLYBAR_NS_END
