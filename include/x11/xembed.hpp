#pragma once

#include "common.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

static constexpr uint32_t XEMBED_VERSION = 0;
static constexpr uint32_t XEMBED_MAPPED = (1 << 0);

static constexpr uint32_t XEMBED_EMBEDDED_NOTIFY = 0;
static constexpr uint32_t XEMBED_WINDOW_ACTIVATE = 1;
static constexpr uint32_t XEMBED_WINDOW_DEACTIVATE = 2;
static constexpr uint32_t XEMBED_REQUEST_FOCUS = 3;
static constexpr uint32_t XEMBED_FOCUS_IN = 3;
static constexpr uint32_t XEMBED_FOCUS_OUT = 4;
static constexpr uint32_t XEMBED_FOCUS_NEXT = 5;
static constexpr uint32_t XEMBED_FOCUS_PREV = 6;

static constexpr uint32_t XEMBED_FOCUS_CURRENT = 0;
static constexpr uint32_t XEMBED_FOCUS_FIRST = 1;
static constexpr uint32_t XEMBED_FOCUS_LAST = 2;

/**
 * Max XEMBED version supported.
 */
static constexpr uint32_t XEMBED_MAX_VERSION = 0;

/**
 * Implementation of parts of the XEMBED spec (as much as needed to get the tray working).
 *
 * Ref: https://specifications.freedesktop.org/xembed-spec/xembed-spec-latest.html
 */
namespace xembed {

class info {
 public:
  void set(const uint32_t* data);

  uint32_t get_version() const;
  uint32_t get_flags() const;

  bool is_mapped() const;

  string to_string() const;

 protected:
  uint32_t version;
  uint32_t flags;
};

bool query(connection& conn, xcb_window_t win, info& data);
void send_message(connection& conn, xcb_window_t target, uint32_t message, uint32_t d1, uint32_t d2, uint32_t d3);
void send_focus_event(connection& conn, xcb_window_t target);
void notify_embedded(connection& conn, xcb_window_t win, xcb_window_t embedder, uint32_t version);
void notify_activated(connection& conn, xcb_window_t win);
void notify_deactivated(connection& conn, xcb_window_t win);
void notify_focused(connection& conn, xcb_window_t win, uint32_t focus_type);
void notify_unfocused(connection& conn, xcb_window_t win);
void unembed(connection& conn, xcb_window_t win, xcb_window_t root);
} // namespace xembed

POLYBAR_NS_END
