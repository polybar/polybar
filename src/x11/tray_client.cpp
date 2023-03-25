#include "x11/tray_client.hpp"

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>

#include "utils/memory.hpp"
#include "x11/connection.hpp"
#include "x11/ewmh.hpp"
#include "x11/winspec.hpp"
#include "xpp/pixmap.hpp"

POLYBAR_NS

namespace tray {

/*
 * The client window is embedded into a wrapper window with identical, size, depth, and visual.
 * This wrapper window is used to paint the background of the icon (also dealing with transparent backgrounds through
 * pseudo-transparency).
 *
 * True transprency is currently not supported here because it cannot be achieved with external compositors (those only
 * seem to work for top-level windows) and has to be implemented by hand.
 */
client::client(
    const logger& log, connection& conn, xcb_window_t parent, xcb_window_t win, size s, rgba desired_background)
    : m_log(log)
    , m_connection(conn)
    , m_name(ewmh_util::get_wm_name(win))
    , m_client(win)
    , m_size(s)
    , m_desired_background(desired_background)
    , m_background_manager(background_manager::make()) {
  auto geom = conn.get_geometry(win);
  auto attrs = conn.get_window_attributes(win);
  int client_depth = geom->depth;
  auto client_visual = attrs->visual;
  auto client_colormap = attrs->colormap;

  m_log.trace("%s: depth: %u, width: %u, height: %u", name(), client_depth, geom->width, geom->height);

  /*
   * Create embedder window for tray icon
   *
   * The embedder window inherits the depth, visual and color map from the icon window in order for reparenting to
   * always work, even if the icon window uses ParentRelative for some of its pixmaps (back pixmap or border pixmap).
   */
  // clang-format off
  m_wrapper = winspec(conn)
    << cw_size(s.w, s.h)
    << cw_pos(0, 0)
    << cw_depth(client_depth)
    << cw_visual(client_visual)
    << cw_parent(parent)
    << cw_class(XCB_WINDOW_CLASS_INPUT_OUTPUT)
    // The X server requires the border pixel to be defined if the depth doesn't match the parent (bar) window
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

  try {
    m_pixmap = m_connection.generate_id();
    m_connection.create_pixmap_checked(client_depth, m_pixmap, m_wrapper, s.w, s.h);
  } catch (const std::exception& err) {
    m_pixmap = XCB_NONE;
    m_log.err("Failed to create pixmap for tray background (err: %s)", err.what());
    throw;
  }

  try {
    m_connection.change_window_attributes_checked(m_wrapper, XCB_CW_BACK_PIXMAP, &m_pixmap);
  } catch (const std::exception& err) {
    m_log.err("Failed to set tray window back pixmap (%s)", err.what());
    throw;
  }

  xcb_visualtype_t* visual = m_connection.visual_type_for_id(client_visual);
  if (!visual) {
    throw std::runtime_error("Failed to get root visual for tray background");
  }

  m_surface = make_unique<cairo::xcb_surface>(m_connection, m_pixmap, visual, s.w, s.h);
  m_context = make_unique<cairo::context>(*m_surface, m_log);

  observe_background();
}

client::~client() {
  if (m_client != XCB_NONE) {
    xembed::unembed(m_connection, m_client, m_connection.root());
  }

  if (m_wrapper != XCB_NONE) {
    m_connection.destroy_window(m_wrapper);
  }

  if (m_pixmap != XCB_NONE) {
    m_connection.free_pixmap(m_pixmap);
  }
}

string client::name() const {
  return "client(" + m_connection.id(m_client) + ", " + m_name + ")";
}

unsigned int client::width() const {
  return m_size.w;
}

unsigned int client::height() const {
  return m_size.h;
}

void client::clear_window() const {
  if (!mapped()) {
    return;
  }

  // Do not produce Expose events for the embedder because that triggers an infinite loop.
  m_connection.clear_area_checked(0, embedder(), 0, 0, width(), height());

  auto send_visibility = [&](uint8_t state) {
    xcb_visibility_notify_event_t evt{};
    evt.response_type = XCB_VISIBILITY_NOTIFY;
    evt.window = client_window();
    evt.state = state;

    m_connection.send_event_checked(
        true, client_window(), XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<const char*>(&evt));
  };

  send_visibility(XCB_VISIBILITY_FULLY_OBSCURED);
  send_visibility(XCB_VISIBILITY_UNOBSCURED);

  m_connection.clear_area_checked(1, client_window(), 0, 0, width(), height());
}

void client::update_client_attributes() const {
  uint32_t configure_mask = 0;
  std::array<uint32_t, 32> configure_values{};
  xcb_params_cw_t configure_params{};

  XCB_AUX_ADD_PARAM(
      &configure_mask, &configure_params, event_mask, XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY);

  connection::pack_values(configure_mask, &configure_params, configure_values);

  m_log.trace("%s: Update client window", name());
  m_connection.change_window_attributes_checked(client_window(), configure_mask, configure_values.data());
}

void client::reparent() const {
  m_log.trace("%s: Reparent client", name());
  m_connection.reparent_window_checked(client_window(), embedder(), 0, 0);
}

/**
 * Is this the client for the given client window
 */
bool client::match(const xcb_window_t& win) const {
  return win == m_client;
}

/**
 * Get client window mapped state
 */
bool client::mapped() const {
  return m_mapped;
}

/**
 * Set client window mapped state
 */
void client::mapped(bool state) {
  if (m_mapped != state) {
    m_log.trace("%s: set mapped: %i", name(), state);
    m_mapped = state;
  }
}

/**
 * Sets the client window's visibility.
 *
 * Use this to trigger a mapping/unmapping
 */
void client::hidden(bool state) {
  m_hidden = state;
}

/**
 * Whether the current state indicates the client should be mapped.
 */
bool client::should_be_mapped() const {
  if (m_hidden) {
    return false;
  }

  if (is_xembed_supported()) {
    return m_xembed.is_mapped();
  }

  return true;
}

xcb_window_t client::embedder() const {
  return m_wrapper;
}

xcb_window_t client::client_window() const {
  return m_client;
}

void client::query_xembed() {
  m_xembed_supported = xembed::query(m_connection, m_client, m_xembed);

  if (is_xembed_supported()) {
    m_log.trace("%s: %s", name(), get_xembed().to_string());
  } else {
    m_log.trace("%s: no xembed");
  }
}

bool client::is_xembed_supported() const {
  return m_xembed_supported;
}

const xembed::info& client::get_xembed() const {
  return m_xembed;
}

void client::notify_xembed() const {
  if (is_xembed_supported()) {
    m_log.trace("%s: Send embedded notification to client", name());
    xembed::notify_embedded(m_connection, client_window(), embedder(), m_xembed.get_version());
  }
}

void client::add_to_save_set() const {
  m_log.trace("%s: Add client window to the save set", name());
  m_connection.change_save_set_checked(XCB_SET_MODE_INSERT, client_window());
}

/**
 * Make sure that the window mapping state is correct
 */
void client::ensure_state() const {
  bool new_state = should_be_mapped();

  if (new_state == m_mapped) {
    return;
  }

  m_log.trace("%s: ensure_state (hidden=%i, mapped=%i, should_be_mapped=%i)", name(), m_hidden, m_mapped, new_state);

  if (new_state) {
    m_log.trace("%s: Map client", name());
    m_connection.map_window_checked(embedder());
    m_connection.map_window_checked(client_window());
  } else {
    m_log.trace("%s: Unmap client", name());
    m_connection.unmap_window_checked(client_window());
    m_connection.unmap_window_checked(embedder());
  }
}

/**
 * Configure window position
 */
void client::set_position(int x, int y) {
  m_log.trace("%s: moving to (%d, %d)", name(), x, y);

  position new_pos{x, y};

  if (new_pos == m_pos) {
    return;
  }

  m_pos = new_pos;

  uint32_t configure_mask = 0;
  array<uint32_t, 32> configure_values{};
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
  m_connection.configure_window_checked(client_window(), configure_mask, configure_values.data());

  xcb_size_hints_t size_hints{};
  xcb_icccm_size_hints_set_size(&size_hints, false, m_size.w, m_size.h);
  xcb_icccm_set_wm_size_hints(m_connection, client_window(), XCB_ATOM_WM_NORMAL_HINTS, &size_hints);

  // The position has changed, we need a new background slice.
  observe_background();
}

/**
 * Respond to client resize/move requests
 */
void client::configure_notify() const {
  xcb_configure_notify_event_t notify;
  notify.response_type = XCB_CONFIGURE_NOTIFY;
  notify.event = client_window();
  notify.window = client_window();
  notify.override_redirect = false;
  notify.above_sibling = 0;
  notify.x = 0;
  notify.y = 0;
  notify.width = m_size.w;
  notify.height = m_size.h;
  notify.border_width = 0;

  unsigned int mask{XCB_EVENT_MASK_STRUCTURE_NOTIFY};
  m_connection.send_event_checked(false, client_window(), mask, reinterpret_cast<const char*>(&notify));
}

/**
 * Redraw background using the observed background slice.
 */
void client::update_bg() const {
  m_log.trace("%s: Update background", name());

  m_context->clear();

  // Composite background slice with background color.
  if (m_bg_slice) {
    auto root_bg = m_bg_slice->get_surface();
    if (root_bg != nullptr) {
      *m_context << CAIRO_OPERATOR_SOURCE << *root_bg;
      m_context->paint();
      *m_context << CAIRO_OPERATOR_OVER;
    }
  }
  *m_context << m_desired_background;
  m_context->paint();

  m_surface->flush();

  clear_window();
  m_connection.flush();
}

void client::observe_background() {
  // Opaque backgrounds don't require pseudo-transparency
  if (!m_transparent) {
    return;
  }

  xcb_rectangle_t rect{0, 0, static_cast<uint16_t>(m_size.w), static_cast<uint16_t>(m_size.h)};
  m_bg_slice = m_background_manager.observe(rect, embedder());

  update_bg();
}

} // namespace tray

POLYBAR_NS_END
