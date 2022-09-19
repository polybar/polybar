#include "x11/tray_manager.hpp"

#include <xcb/xcb_image.h>

#include <thread>

#include "cairo/context.hpp"
#include "cairo/surface.hpp"
#include "components/config.hpp"
#include "errors.hpp"
#include "events/signal.hpp"
#include "utils/color.hpp"
#include "utils/factory.hpp"
#include "utils/math.hpp"
#include "utils/memory.hpp"
#include "utils/process.hpp"
#include "utils/units.hpp"
#include "x11/ewmh.hpp"
#include "x11/icccm.hpp"
#include "x11/window.hpp"
#include "x11/winspec.hpp"
#include "x11/xembed.hpp"

/*
 * Tray implementation according to the System Tray Protocol.
 *
 * Ref: https://specifications.freedesktop.org/systemtray-spec/systemtray-spec-latest.html
 *
 * This class manages embedded tray icons by placing them on the bar in the correct position; the position itself is
 * requested by the renderer.
 *
 * The tray manager needs to trigger bar updates only when the size of the entire tray changes (e.g. when tray icons are
 * added/removed). EVerything else can be handled without an update.
 */

// ====================================================================================================
//
// TODO: 32-bit visual
//
// _NET_SYSTEM_TRAY_VISUAL visual_id VISUALID/32
//
// The property should be set by the tray manager to indicate the preferred visual for icon windows.
//
// To avoid ambiguity about the colormap to use this visual must either be the default visual for
// the screen or it must be a TrueColor visual. If this property is set to a visual with an alpha
// channel, the tray manager must use the Composite extension to composite the icon against the
// background using PictOpOver.
//
// ====================================================================================================

POLYBAR_NS

tray_manager::tray_manager(
    connection& conn, signal_emitter& emitter, const logger& logger, const bar_settings& bar_opts, on_update on_update)
    : m_connection(conn), m_sig(emitter), m_log(logger), m_bar_opts(bar_opts), m_on_update(on_update) {
  m_connection.attach_sink(this, SINK_PRIORITY_TRAY);
}

tray_manager::~tray_manager() {
  if (m_delaythread.joinable()) {
    m_delaythread.join();
  }
  m_connection.detach_sink(this, SINK_PRIORITY_TRAY);
  deactivate();
}

void tray_manager::setup(const config& conf, const string& section_name) {
  unsigned client_height = m_bar_opts.inner_area().height;

  // Add user-defined padding
  m_opts.spacing = conf.get<unsigned>(section_name, "tray-padding", 0);

  auto maxsize = conf.get<unsigned>(section_name, "tray-maxsize", 16);
  if (client_height > maxsize) {
    m_opts.spacing += (client_height - maxsize) / 2;
    client_height = maxsize;
  }

  // Apply user-defined scaling
  auto scale = conf.get(section_name, "tray-scale", 1.0);
  client_height *= scale;

  m_opts.client_size = {client_height, client_height};

  // Set user-defined foreground and background colors.
  // TODO maybe remove
  m_opts.background = conf.get(section_name, "tray-background", m_bar_opts.background);
  m_opts.foreground = conf.get(section_name, "tray-foreground", m_bar_opts.foreground);

  m_opts.selection_owner = m_bar_opts.x_data.window;

  if (m_bar_opts.x_data.window == XCB_NONE) {
    m_log.err("tray: No bar window found, disabling tray");
    return;
  }

  // Activate the tray manager
  query_atom();
  activate();
}

int tray_manager::get_width() const {
  return m_tray_width;
}

bool tray_manager::is_active() const {
  return m_state == state::ACTIVE;
}

bool tray_manager::is_inactive() const {
  return m_state == state::INACTIVE;
}

bool tray_manager::is_waiting() const {
  return m_state == state::WAITING;
}

bool tray_manager::is_visible() const {
  return is_active() && !m_hidden;
}

/**
 * Activate systray management
 */
void tray_manager::activate() {
  if (is_active()) {
    return;
  }

  m_log.info("tray: Activating tray manager");

  try {
    set_tray_colors();
    set_tray_orientation();
  } catch (const exception& err) {
    m_log.err(err.what());
    m_log.err("Cannot activate tray manager... failed to setup window");
    deactivate();
    return;
  }

  // Attempt to get control of the systray selection
  xcb_window_t other_owner = XCB_NONE;
  if (!acquire_selection(other_owner)) {
    // Transition to WAITING state
    wait_for_selection(other_owner);
    return;
  }

  m_sig.attach(this);

  m_othermanager = XCB_NONE;

  m_state = state::ACTIVE;

  // Send delayed notification
  // TODO try to remove this?
  if (!m_firstactivation) {
    notify_clients();
  } else {
    notify_clients_delayed();
  }

  m_firstactivation = false;
}

/**
 * Transitions tray manager to WAITING state
 *
 * @param other window id for current selection owner
 */
void tray_manager::wait_for_selection(xcb_window_t other) {
  if (is_waiting() || other == XCB_NONE) {
    return;
  }

  m_log.info("tray: Waiting for systray selection (current owner: %s)", m_connection.id(other));

  m_sig.detach(this);

  m_othermanager = other;
  track_selection_owner(other);

  m_log.trace("tray: Unembed clients");
  m_clients.clear();

  m_connection.flush();

  m_state = state::WAITING;

  reconfigure_window();
}

/**
 * Deactivate systray management
 */
void tray_manager::deactivate() {
  if (is_inactive()) {
    return;
  }

  m_log.info("tray: Deactivating tray manager");

  m_sig.detach(this);

  // Unset selection owner if we currently own the atom
  if (!m_connection.connection_has_error() && is_active()) {
    m_log.trace("tray: Unset selection owner");
    m_connection.set_selection_owner(XCB_NONE, m_atom, XCB_CURRENT_TIME);
  }

  m_log.trace("tray: Unembed clients");
  m_clients.clear();

  m_connection.flush();

  m_othermanager = XCB_NONE;

  m_state = state::INACTIVE;

  reconfigure_window();
}

/**
 * Reconfigure tray
 */
void tray_manager::reconfigure() {
  if (!m_opts.selection_owner) {
    return;
  }

  reconfigure_window();

  try {
    reconfigure_clients();
  } catch (const exception& err) {
    m_log.err("Failed to reconfigure tray clients (%s)", err.what());
  }

  m_connection.flush();
}

/**
 * Reconfigure container window
 *
 * TODO should we call update_width directly?
 */
void tray_manager::reconfigure_window() {
  m_log.trace("tray: Reconfigure window (hidden=%i, clients=%i)", m_hidden, m_clients.size());
  update_width();
}

/**
 * TODO make sure this is always called when m_clients changes
 */
void tray_manager::update_width() {
  unsigned new_width = calculate_w();
  if (m_tray_width != new_width) {
    m_tray_width = new_width;
    m_log.trace("tray: new width (width: %d, clients: %d)", m_tray_width, m_clients.size());
    m_on_update();
  }
}

/**
 * Reconfigure clients
 */
void tray_manager::reconfigure_clients() {
  m_log.trace("tray: Reconfigure clients");

  int x = calculate_x() + m_opts.spacing;

  for (auto it = m_clients.rbegin(); it != m_clients.rend(); it++) {
    try {
      it->ensure_state();
      // TODO skip if the client isn't mapped
      it->reconfigure(x, calculate_client_y());

      x += m_opts.client_size.w + m_opts.spacing;
    } catch (const xpp::x::error::window& err) {
      m_log.err("Failed to reconfigure %s, removing ... (%s)", it->name(), err.what());
      remove_client(*it);
    }
  }
}

/**
 * Refresh the bar window by clearing it along with each client window
 */
void tray_manager::refresh_window() {
  if (!is_visible()) {
    return;
  }

  m_log.trace("tray: Refreshing clients");

  for (auto& client : m_clients) {
    try {
      if (client.mapped()) {
        client.clear_window();
      }
    } catch (const std::exception& e) {
      m_log.err("tray: Failed to clear %s (%s)", client.name(), e.what());
    }
  }

  m_connection.flush();
}

/**
 * Redraw window
 *
 * TODO better name
 */
void tray_manager::redraw_window() {
  refresh_window();
}

/**
 * Find the systray selection atom
 */
void tray_manager::query_atom() {
  m_log.trace("tray: Find systray selection atom for the default screen");
  string name{"_NET_SYSTEM_TRAY_S" + to_string(m_connection.default_screen())};
  auto reply = m_connection.intern_atom(false, name.length(), name.c_str());
  m_atom = reply.atom();
}

/**
 * Set color atom used by clients when determing icon theme
 */
void tray_manager::set_tray_colors() {
  m_log.trace("tray: Set _NET_SYSTEM_TRAY_COLORS to %x", m_opts.foreground);

  auto r = m_opts.foreground.red_i();
  auto g = m_opts.foreground.green_i();
  auto b = m_opts.foreground.blue_i();

  const uint16_t r16 = (r << 8) | r;
  const uint16_t g16 = (g << 8) | g;
  const uint16_t b16 = (b << 8) | b;

  const uint32_t colors[12] = {
      r16, g16, b16, // normal
      r16, g16, b16, // error
      r16, g16, b16, // warning
      r16, g16, b16, // success
  };

  m_connection.change_property(
      XCB_PROP_MODE_REPLACE, m_opts.selection_owner, _NET_SYSTEM_TRAY_COLORS, XCB_ATOM_CARDINAL, 32, 12, colors);
}

/**
 * Set the _NET_SYSTEM_TRAY_ORIENTATION atom
 */
void tray_manager::set_tray_orientation() {
  const uint32_t orientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
  m_log.trace("tray: Set _NET_SYSTEM_TRAY_ORIENTATION to 0x%x", orientation);
  m_connection.change_property_checked(XCB_PROP_MODE_REPLACE, m_opts.selection_owner, _NET_SYSTEM_TRAY_ORIENTATION,
      XCB_ATOM_CARDINAL, 32, 1, &orientation);
}

/**
 * Acquire the systray selection
 *
 * @param other_owner is set to the current owner if the function fails
 * @returns Whether we acquired the selection
 */
bool tray_manager::acquire_selection(xcb_window_t& other_owner) {
  other_owner = XCB_NONE;
  xcb_window_t owner = m_connection.get_selection_owner(m_atom).owner();

  if (owner == m_opts.selection_owner) {
    m_log.trace("tray: Already managing the systray selection");
    return true;
  } else if (owner == XCB_NONE) {
    m_log.trace("tray: Change selection owner to %s", m_connection.id(m_opts.selection_owner));
    m_connection.set_selection_owner_checked(m_opts.selection_owner, m_atom, XCB_CURRENT_TIME);
    if (m_connection.get_selection_owner_unchecked(m_atom)->owner != m_opts.selection_owner) {
      throw application_error("Failed to get control of the systray selection");
    }
    return true;
  } else {
    other_owner = owner;
    m_log.warn("Systray selection already managed (window=%s)", m_connection.id(owner));
    return false;
  }
}

/**
 * Notify pending clients about the new systray MANAGER
 */
void tray_manager::notify_clients() {
  if (is_active()) {
    m_log.info("tray: Notifying pending tray clients");
    auto message = m_connection.make_client_message(MANAGER, m_connection.root());
    message.data.data32[0] = XCB_CURRENT_TIME;
    message.data.data32[1] = m_atom;
    message.data.data32[2] = m_opts.selection_owner;
    m_connection.send_client_message(message, m_connection.root());
  }
}

/**
 * Send delayed notification to pending clients
 */
void tray_manager::notify_clients_delayed() {
  if (m_delaythread.joinable()) {
    m_delaythread.join();
  }
  m_delaythread = thread([this]() {
    this_thread::sleep_for(1s);
    notify_clients();
  });
}

/**
 * Track changes to the given selection owner
 * If it gets destroyed or goes away we can reactivate the tray_manager
 */
void tray_manager::track_selection_owner(xcb_window_t owner) {
  if (owner != XCB_NONE) {
    const unsigned mask{XCB_CW_EVENT_MASK};
    const unsigned values[]{XCB_EVENT_MASK_STRUCTURE_NOTIFY};
    m_connection.change_window_attributes(owner, mask, values);
  }
}

/**
 * Process client docking request
 */
void tray_manager::process_docking_request(xcb_window_t win) {
  m_log.info("tray: Processing docking request from '%s' (%s)", ewmh_util::get_wm_name(win), m_connection.id(win));

  try {
    tray_client client(m_log, m_connection, m_opts.selection_owner, win, m_opts.client_size);

    try {
      client.query_xembed();
    } catch (const xpp::x::error::window& err) {
      m_log.err("Failed to query _XEMBED_INFO, removing %s ... (%s)", client.name(), err.what());
      return;
    }

    client.update_client_attributes();

    client.reparent();

    client.add_to_save_set();

    client.notify_xembed();

    client.ensure_state();

    m_clients.emplace_back(std::move(client));
  } catch (const std::exception& err) {
    m_log.err("tray: Failed to setup tray client removing... (%s)", err.what());
    return;
  }
}

/**
 * Calculate x position of tray window
 */
int tray_manager::calculate_x() const {
  return m_bar_opts.inner_area(false).x + m_pos.x;
}

int tray_manager::calculate_y() const {
  return m_bar_opts.inner_area(false).y + m_pos.y;
}

unsigned tray_manager::calculate_w() const {
  unsigned width = m_opts.spacing;
  unsigned count{0};
  for (auto& client : m_clients) {
    if (client.mapped()) {
      count++;
      width += m_opts.spacing + m_opts.client_size.w;
    }
  }
  return count ? width : 0;
}

/**
 * Calculate y position of client window
 */
int tray_manager::calculate_client_y() {
  return (m_bar_opts.inner_area(true).height - m_opts.client_size.h) / 2;
}

/**
 * Check if the given window is embedded.
 *
 * The given window ID can be the ID of the wrapper or the embedded window
 */
bool tray_manager::is_embedded(const xcb_window_t& win) {
  return find_client(win) != nullptr;
}

/**
 * Find tray client object from the wrapper or embedded window
 */
tray_client* tray_manager::find_client(const xcb_window_t& win) {
  auto client = std::find_if(m_clients.begin(), m_clients.end(),
      [win](const auto& client) { return client.match(win) || client.embedder() == win; });

  if (client == m_clients.end()) {
    return nullptr;
  } else {
    return &(*client);
  }
}

/**
 * Remove tray client
 */
void tray_manager::remove_client(const tray_client& client) {
  remove_client(client.client());
}

/**
 * Remove tray client by window
 */
void tray_manager::remove_client(xcb_window_t win) {
  auto old_size = m_clients.size();
  m_clients.erase(
      std::remove_if(m_clients.begin(), m_clients.end(), [win](const auto& client) { return client.match(win); }));

  if (old_size != m_clients.size()) {
    tray_manager::reconfigure();
  }
}

bool tray_manager::change_visibility(bool visible) {
  if (!is_active() || m_hidden == !visible) {
    return false;
  }

  m_log.trace("tray: visibility_change (new_state)", visible ? "visible" : "hidden");

  m_hidden = !visible;

  for (auto& client : m_clients) {
    client.hidden(m_hidden);
    client.ensure_state();
  }

  if (!m_hidden) {
    redraw_window();
  }

  m_connection.flush();

  return true;
}

/**
 * Event callback : XCB_EXPOSE
 */
void tray_manager::handle(const evt::expose& evt) {
  if (is_active() && !m_clients.empty() && evt->count == 0) {
    redraw_window();
  }
}

/**
 * Event callback : XCB_CLIENT_MESSAGE
 */
void tray_manager::handle(const evt::client_message& evt) {
  if (!is_active()) {
    return;
  }

  // Our selection owner window was deleted
  if (evt->type == WM_PROTOCOLS && evt->data.data32[0] == WM_DELETE_WINDOW && evt->window == m_opts.selection_owner) {
    m_log.notice("Received WM_DELETE for selection owner");
    deactivate();
  } else if (evt->type == _NET_SYSTEM_TRAY_OPCODE && evt->format == 32) {
    m_log.trace("tray: Received client_message");

    // Docking request
    if (SYSTEM_TRAY_REQUEST_DOCK == evt->data.data32[1]) {
      xcb_window_t win = evt->data.data32[2];
      if (is_embedded(win)) {
        m_log.warn("Tray client %s already embedded, ignoring request...", m_connection.id(win));
      } else {
        process_docking_request(win);
      }
    }
  }
}

/**
 * Event callback : XCB_CONFIGURE_REQUEST
 *
 * Called when a tray client thinks he's part of the free world and
 * wants to reconfigure its window. This is of course nothing we appreciate
 * so we return an answer that'll put him in place.
 */
void tray_manager::handle(const evt::configure_request& evt) {
  if (is_active() && is_embedded(evt->window)) {
    auto client = find_client(evt->window);
    try {
      m_log.trace("%s: Client configure request", client->name());
      client->configure_notify();
    } catch (const xpp::x::error::window& err) {
      m_log.err("Failed to reconfigure %s, removing... (%s)", client->name(), err.what());
      remove_client(evt->window);
    }
  }
}

/**
 * @see tray_manager::handle(const evt::configure_request&);
 */
void tray_manager::handle(const evt::resize_request& evt) {
  if (is_active() && is_embedded(evt->window)) {
    auto client = find_client(evt->window);
    try {
      m_log.trace("%s: Client resize request", client->name());
      client->configure_notify();
    } catch (const xpp::x::error::window& err) {
      m_log.err("Failed to reconfigure %s, removing... (%s)", client->name(), err.what());
      remove_client(evt->window);
    }
  }
}

/**
 * Event callback : XCB_SELECTION_CLEAR
 */
void tray_manager::handle(const evt::selection_clear& evt) {
  if (is_inactive()) {
    return;
  } else if (evt->selection != m_atom) {
    return;
  } else if (evt->owner != m_opts.selection_owner) {
    return;
  }

  m_log.warn("Lost systray selection, deactivating...");
  wait_for_selection(m_connection.get_selection_owner(m_atom).owner());
}

/**
 * Event callback : XCB_PROPERTY_NOTIFY
 */
void tray_manager::handle(const evt::property_notify& evt) {
  if (!is_active()) {
    return;
  }

  if (evt->atom != _XEMBED_INFO) {
    return;
  }

  auto client = find_client(evt->window);

  if (!client) {
    return;
  }

  m_log.trace("%s: _XEMBED_INFO", client->name());

  if (evt->state == XCB_PROPERTY_NEW_VALUE) {
    m_log.trace("tray: _XEMBED_INFO value has changed");
  }

  try {
    client->query_xembed();
  } catch (const xpp::x::error::window& err) {
    m_log.err("Failed to query _XEMBED_INFO, removing %s ... (%s)", client->name(), err.what());
    remove_client(*client);
    return;
  }

  client->ensure_state();
}

/**
 * Event callback : XCB_REPARENT_NOTIFY
 */
void tray_manager::handle(const evt::reparent_notify& evt) {
  if (!is_active()) {
    return;
  }

  auto client = find_client(evt->window);

  if (!client) {
    return;
  }

  // Tray client was reparented to another window
  if (evt->parent != client->embedder()) {
    m_log.info("%s: Received reparent_notify for client, remove...", client->name());
    remove_client(*client);
  }
}

/**
 * Event callback : XCB_DESTROY_NOTIFY
 */
void tray_manager::handle(const evt::destroy_notify& evt) {
  if (is_waiting() && evt->window == m_othermanager) {
    m_log.info("Systray selection unmanaged... re-activating");
    activate();
  } else if (is_active() && is_embedded(evt->window)) {
    m_log.info("tray: Received destroy_notify for client, remove...");
    remove_client(evt->window);
    redraw_window();
  }
}

/**
 * Event callback : XCB_MAP_NOTIFY
 */
void tray_manager::handle(const evt::map_notify& evt) {
  if (is_active() && evt->window == m_opts.selection_owner) {
    m_log.trace("tray: Received map_notify for selection owner");
    redraw_window();
  } else if (is_embedded(evt->window)) {
    auto client = find_client(evt->window);

    // If we received a notification on the wrapped window, we don't want to do anything.
    if (client->embedder() != evt->window) {
      return;
    }

    m_log.trace("%s: Received map_notify", client->name());

    if (!client->mapped()) {
      client->mapped(true);
      reconfigure();
    }
  }
}

/**
 * Event callback : XCB_UNMAP_NOTIFY
 */
void tray_manager::handle(const evt::unmap_notify& evt) {
  if (is_active() && is_embedded(evt->window)) {
    auto client = find_client(evt->window);

    // If we received a notification on the wrapped window, we don't want to do anything.
    if (client->embedder() != evt->window) {
      return;
    }

    m_log.trace("%s: Received unmap_notify", client->name());

    if (client->mapped()) {
      client->mapped(false);
      reconfigure();
    }
  }
}

// TODO maybe remove signal
bool tray_manager::on(const signals::ui::update_background&) {
  redraw_window();

  return false;
}

bool tray_manager::on(const signals::ui_tray::tray_pos_change& evt) {
  int new_x = std::max(0, std::min(evt.cast(), (int)(m_bar_opts.size.w - m_tray_width)));

  if (new_x != m_pos.x) {
    m_pos.x = new_x;
    reconfigure();
  }

  return true;
}

bool tray_manager::on(const signals::ui_tray::tray_visibility& evt) {
  return change_visibility(evt.cast());
}

POLYBAR_NS_END
