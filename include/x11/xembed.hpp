#pragma once

#include "common.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

#define XEMBED_VERSION 0
#define XEMBED_MAPPED (1 << 0)

#define XEMBED_EMBEDDED_NOTIFY 0
#define XEMBED_WINDOW_ACTIVATE 1
#define XEMBED_WINDOW_DEACTIVATE 2
#define XEMBED_REQUEST_FOCUS 3
#define XEMBED_FOCUS_IN 3
#define XEMBED_FOCUS_OUT 4
#define XEMBED_FOCUS_NEXT 5
#define XEMBED_FOCUS_PREV 6

#define XEMBED_FOCUS_CURRENT 0
#define XEMBED_FOCUS_FIRST 1
#define XEMBED_FOCUS_LAST 1

struct xembed_data {
  unsigned long version;
  unsigned long flags;
  xcb_timestamp_t time;
  xcb_atom_t xembed;
  xcb_atom_t xembed_info;
};

namespace xembed {
  xembed_data* query(connection& conn, xcb_window_t win, xembed_data* data);
  void send_message(connection& conn, xcb_window_t target, long message, long d1, long d2, long d3);
  void send_focus_event(connection& conn, xcb_window_t target);
  void notify_embedded(connection& conn, xcb_window_t win, xcb_window_t embedder, long version);
  void notify_activated(connection& conn, xcb_window_t win);
  void notify_deactivated(connection& conn, xcb_window_t win);
  void notify_focused(connection& conn, xcb_window_t win, long focus_type);
  void notify_unfocused(connection& conn, xcb_window_t win);
  void unembed(connection& conn, xcb_window_t win, xcb_window_t root);
}

POLYBAR_NS_END
