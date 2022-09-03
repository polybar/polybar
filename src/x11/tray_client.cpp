#include "x11/tray_client.hpp"

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "utils/memory.hpp"
#include "x11/connection.hpp"
#include "x11/winspec.hpp"

POLYBAR_NS

/*
 * TODO proper background of wrapper window
 *
 * Do first possible:
 *
 * 1. Use PARENT_RELATIVE if tray window depths, etc. matches the bar window
 * 2. Use pseudo-transparency when activated (make sure the depths match)
 * 3. Use background color
 */
tray_client::tray_client(const logger& log, connection& conn, xcb_window_t tray, xcb_window_t win, size s)
    : m_log(log), m_connection(conn), m_client(win), m_size(s) {
  auto geom = conn.get_geometry(win);
  auto attrs = conn.get_window_attributes(win);
  int client_depth = geom->depth;
  auto client_visual = attrs->visual;
  auto client_colormap = attrs->colormap;

  m_log.trace("tray(%s): depth: %u, width: %u, height: %u", conn.id(win), client_depth, geom->width, geom->height);

  /*
   * Create embedder window for tray icon
   *
   * The embedder window inherits the depth, visual and color map from the icon window in order for reparenting to
   * always work, even if the icon window uses ParentRelative for some of its pixmaps (back pixmap or border pixmap).
   */
  // clang-format off
  m_wrapper = winspec(conn)
    << cw_size(s.h, s.w)
    << cw_pos(0, 0)
    << cw_depth(client_depth)
    << cw_visual(client_visual)
    << cw_parent(tray)
    << cw_class(XCB_WINDOW_CLASS_INPUT_OUTPUT)
    // TODO add proper pixmap
    << cw_params_back_pixmap(XCB_PIXMAP_NONE)
    // << cw_params_back_pixel(0x00ff00)
    // The X server requires the border pixel to be defined if the depth doesn't match the parent window
    << cw_params_border_pixel(conn.screen()->black_pixel)
    << cw_params_backing_store(XCB_BACKING_STORE_WHEN_MAPPED)
    << cw_params_save_under(true)
    << cw_params_event_mask(XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
        | XCB_EVENT_MASK_PROPERTY_CHANGE
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_EXPOSURE)
    << cw_params_colormap(client_colormap)
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

tray_client::tray_client(tray_client&& c) : m_log(c.m_log), m_connection(c.m_connection) {
  std::swap(m_wrapper, c.m_wrapper);
  std::swap(m_client, c.m_client);
  std::swap(m_xembed_supported, c.m_xembed_supported);
  std::swap(m_xembed, c.m_xembed);
  std::swap(m_mapped, c.m_mapped);
  std::swap(m_hidden, c.m_hidden);
  std::swap(m_size, c.m_size);
}

tray_client& tray_client::operator=(tray_client&& c) {
  m_log = c.m_log;
  m_connection = c.m_connection;
  std::swap(m_wrapper, c.m_wrapper);
  std::swap(m_client, c.m_client);
  std::swap(m_xembed_supported, c.m_xembed_supported);
  std::swap(m_xembed, c.m_xembed);
  std::swap(m_mapped, c.m_mapped);
  std::swap(m_hidden, c.m_hidden);
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

void tray_client::update_client_attributes() const {
  uint32_t configure_mask = 0;
  std::array<uint32_t, 32> configure_values{};
  xcb_params_cw_t configure_params{};

  XCB_AUX_ADD_PARAM(
      &configure_mask, &configure_params, event_mask, XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY);

  // TODO
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, back_pixel, 0xff0000ff);
  // TODO
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, back_pixmap, XCB_PIXMAP_NONE);

  connection::pack_values(configure_mask, &configure_params, configure_values);

  m_log.trace("tray(%s): Update client window", m_connection.id(client()));
  m_connection.change_window_attributes_checked(client(), configure_mask, configure_values.data());
}

void tray_client::reparent() const {
  m_log.trace("tray(%s): Reparent client", m_connection.id(client()));
  m_connection.reparent_window_checked(client(), embedder(), 0, 0);
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
  m_log.trace("tray(%s): set mapped: %i", m_connection.id(client()), state);
  m_mapped = state;
}

/**
 * Sets the client window's visibility.
 *
 * Use this to trigger a mapping/unmapping
 */
void tray_client::hidden(bool state) {
  m_hidden = state;
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

void tray_client::notify_xembed() const {
  if (is_xembed_supported()) {
    m_log.trace("tray(%s): Send embbeded notification to client", m_connection.id(client()));
    xembed::notify_embedded(m_connection, client(), embedder(), m_xembed.get_version());
  }
}

void tray_client::add_to_save_set() const {
  m_log.trace("tray(%s): Add client window to the save set", m_connection.id(client()));
  m_connection.change_save_set_checked(XCB_SET_MODE_INSERT, client());
}

/**
 * Make sure that the window mapping state is correct
 */
void tray_client::ensure_state() const {
  bool should_be_mapped = true;

  if (is_xembed_supported()) {
    should_be_mapped = m_xembed.is_mapped();
  }

  if (m_hidden) {
    should_be_mapped = false;
  }

  m_log.trace("tray(%s): ensure_state (hidden=%i, mapped=%i, should_be_mapped=%i)", m_connection.id(client()), m_hidden,
      m_mapped, should_be_mapped);

  if (should_be_mapped) {
    m_log.trace("tray(%s): Map client", m_connection.id(client()));
    m_connection.map_window_checked(embedder());
    m_connection.map_window_checked(client());
  } else {
    m_log.trace("tray(%s): Unmap client", m_connection.id(client()));
    m_connection.unmap_window_checked(client());
    m_connection.unmap_window_checked(embedder());
  }
}

/**
 * Configure window size
 */
void tray_client::reconfigure(int x, int y) const {
  m_log.trace("tray(%s): moving to (%d, %d)", m_connection.id(client()), x, y);

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
void tray_client::configure_notify() const {
  xcb_configure_notify_event_t notify;
  notify.response_type = XCB_CONFIGURE_NOTIFY;
  notify.event = client();
  notify.window = client();
  notify.override_redirect = false;
  notify.above_sibling = 0;
  notify.x = 0;
  notify.y = 0;
  notify.width = m_size.w;
  notify.height = m_size.h;
  notify.border_width = 0;

  unsigned int mask{XCB_EVENT_MASK_STRUCTURE_NOTIFY};
  m_connection.send_event_checked(false, client(), mask, reinterpret_cast<const char*>(&notify));
}

POLYBAR_NS_END
