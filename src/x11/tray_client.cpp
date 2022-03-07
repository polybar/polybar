#include "x11/tray_client.hpp"

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "utils/memory.hpp"
#include "x11/connection.hpp"
#include "x11/winspec.hpp"

POLYBAR_NS

// TODO create wrapper window
tray_client::tray_client(const logger& log, connection& conn, xcb_window_t tray, xcb_window_t win, size s)
    : m_log(log), m_connection(conn), m_client(win), m_size(s) {
  auto geom = conn.get_geometry(win);
  auto attrs = conn.get_window_attributes(win);
  int depth = geom->depth;
  xcb_visualid_t visual = attrs->visual;
  m_log.trace("tray(%s): depth: %u, width: %u, height: %u, visual: 0x%x", conn.id(win), depth, geom->width, geom->height, visual);

  // clang-format off
  m_wrapper = winspec(conn)
    << cw_size(s.h, s.w)
    << cw_pos(0, 0)
    // TODO fix BadMatch error for redshift-gtk window
    // << cw_depth(depth)
    // << cw_visual(visual)
    << cw_parent(tray)
    // TODO add proper pixmap
    << cw_params_back_pixmap(XCB_PIXMAP_NONE)
    // << cw_class(XCB_WINDOW_CLASS_INPUT_OUTPUT)
    << cw_params_backing_store(XCB_BACKING_STORE_WHEN_MAPPED)
    << cw_params_event_mask(XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
        | XCB_EVENT_MASK_PROPERTY_CHANGE
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_EXPOSURE)
    // << cw_flush(false);
    // TODO Make unchecked
    << cw_flush(true);
  // clang-format on
}

tray_client::~tray_client() {
  if (m_client != XCB_NONE) {
    xembed::unembed(m_connection, m_client, m_connection.root());
  }

  if (m_wrapper != XCB_NONE) {
    m_connection.destroy_window(m_wrapper);
  }
}

tray_client::tray_client(tray_client&& c) : m_log(c.m_log), m_connection(c.m_connection), m_size(c.m_size) {
  std::swap(m_wrapper, c.m_wrapper);
  std::swap(m_client, c.m_client);
}

tray_client& tray_client::operator=(tray_client&& c) {
  m_log = c.m_log;
  m_connection = c.m_connection;
  std::swap(m_wrapper, c.m_wrapper);
  std::swap(m_client, c.m_client);
  std::swap(m_size, c.m_size);
  return *this;
}

unsigned int tray_client::width() const {
  return m_size.w;
}

unsigned int tray_client::height() const {
  return m_size.h;
}

void tray_client::clear_window() const {
  m_connection.clear_area_checked(1, client(), 0, 0, width(), height());
}

/**
 * Is this the client for the given client window
 */
bool tray_client::match(const xcb_window_t& win) const {
  return win == m_client;
}

/**
 * Get client window mapped state
 */
bool tray_client::mapped() const {
  return m_mapped;
}

/**
 * Set client window mapped state
 */
void tray_client::mapped(bool state) {
  m_mapped = state;
}

xcb_window_t tray_client::embedder() const {
  return m_wrapper;
}

xcb_window_t tray_client::client() const {
  return m_client;
}

void tray_client::query_xembed() {
  m_xembed_supported = xembed::query(m_connection, m_client, m_xembed);
}

bool tray_client::is_xembed_supported() const {
  return m_xembed_supported;
}

const xembed::info& tray_client::get_xembed() const {
  return m_xembed;
}

/**
 * Make sure that the window mapping state is correct
 */
void tray_client::ensure_state() const {
  // TODO correctly map/unmap wrapper
  bool should_be_mapped = true;

  if (is_xembed_supported()) {
    should_be_mapped = m_xembed.is_mapped();
  }

  if (!mapped() && should_be_mapped) {
    m_connection.map_window_checked(embedder());
    m_connection.map_window_checked(client());
  } else if (mapped() && !should_be_mapped) {
    m_connection.unmap_window_checked(embedder());
    m_connection.unmap_window_checked(client());
  }
}

/**
 * Configure window size
 */
void tray_client::reconfigure(int x, int y) const {
  m_log.trace("tray(%s): moving to (%d, %d)", m_connection.id(client()), x, y);

  // TODO correctly reconfigure wrapper + client
  uint32_t configure_mask = 0;
  std::array<uint32_t, 32> configure_values{};
  xcb_params_configure_window_t configure_params{};

  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, width, m_size.w);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, height, m_size.h);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, x, x);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, y, y);
  connection::pack_values(configure_mask, &configure_params, configure_values);
  m_connection.configure_window_checked(embedder(), configure_mask, configure_values.data());

  configure_mask = 0;
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, width, m_size.w);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, height, m_size.h);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, x, 0);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, y, 0);
  connection::pack_values(configure_mask, &configure_params, configure_values);
  m_connection.configure_window_checked(client(), configure_mask, configure_values.data());
}

/**
 * Respond to client resize/move requests
 */
void tray_client::configure_notify(int x, int y) const {
  // TODO remove x and y position. The position will always be (0,0)
  xcb_configure_notify_event_t notify;
  notify.response_type = XCB_CONFIGURE_NOTIFY;
  notify.event = m_client;
  notify.window = m_client;
  notify.override_redirect = false;
  notify.above_sibling = 0;
  notify.x = x;
  notify.y = y;
  notify.width = m_size.w;
  notify.height = m_size.h;
  notify.border_width = 0;

  unsigned int mask{XCB_EVENT_MASK_STRUCTURE_NOTIFY};
  m_connection.send_event_checked(false, m_client, mask, reinterpret_cast<const char*>(&notify));
}

POLYBAR_NS_END
