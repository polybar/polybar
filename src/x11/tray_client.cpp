#include "x11/tray_client.hpp"

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "utils/memory.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

tray_client::tray_client(connection& conn, xcb_window_t win, unsigned int w, unsigned int h)
    : m_connection(conn), m_window(win), m_width(w), m_height(h) {}

tray_client::~tray_client() {
  xembed::unembed(m_connection, window(), m_connection.root());
}

unsigned int tray_client::width() const {
  return m_width;
}

unsigned int tray_client::height() const {
  return m_height;
}

void tray_client::clear_window() const {
  m_connection.clear_area_checked(1, window(), 0, 0, width(), height());
}

/**
 * Match given window against client window
 */
bool tray_client::match(const xcb_window_t& win) const {
  return win == m_window;
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

/**
 * Get client window
 */
xcb_window_t tray_client::window() const {
  return m_window;
}

void tray_client::query_xembed() {
  m_xembed_supported = xembed::query(m_connection, m_window, m_xembed);
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
  bool should_be_mapped = true;

  if (is_xembed_supported()) {
    should_be_mapped = m_xembed.is_mapped();
  }

  if (!mapped() && should_be_mapped) {
    m_connection.map_window_checked(window());
  } else if (mapped() && !should_be_mapped) {
    m_connection.unmap_window_checked(window());
  }
}

/**
 * Configure window size
 */
void tray_client::reconfigure(int x, int y) const {
  unsigned int configure_mask = 0;
  unsigned int configure_values[7];
  xcb_params_configure_window_t configure_params{};

  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, width, m_width);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, height, m_height);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, x, x);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, y, y);

  connection::pack_values(configure_mask, &configure_params, configure_values);
  m_connection.configure_window_checked(window(), configure_mask, configure_values);
}

/**
 * Respond to client resize requests
 */
void tray_client::configure_notify(int x, int y) const {
  auto notify = memory_util::make_malloc_ptr<xcb_configure_notify_event_t, 32_z>();
  notify->response_type = XCB_CONFIGURE_NOTIFY;
  notify->event = m_window;
  notify->window = m_window;
  notify->override_redirect = false;
  notify->above_sibling = 0;
  notify->x = x;
  notify->y = y;
  notify->width = m_width;
  notify->height = m_height;
  notify->border_width = 0;

  unsigned int mask{XCB_EVENT_MASK_STRUCTURE_NOTIFY};
  m_connection.send_event_checked(false, m_window, mask, reinterpret_cast<const char*>(notify.get()));
}

POLYBAR_NS_END
