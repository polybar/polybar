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

/**
 * Create instance
 */
tray_manager::make_type tray_manager::make(const bar_settings& bar_opts) {
  return std::make_unique<tray_manager>(connection::make(), signal_emitter::make(), logger::make(), bar_opts);
}

tray_manager::tray_manager(
    connection& conn, signal_emitter& emitter, const logger& logger, const bar_settings& bar_opts)
    : m_connection(conn), m_sig(emitter), m_log(logger), m_bar_opts(bar_opts) {
  m_connection.attach_sink(this, SINK_PRIORITY_TRAY);
}

tray_manager::~tray_manager() {
  if (m_delaythread.joinable()) {
    m_delaythread.join();
  }
  m_connection.detach_sink(this, SINK_PRIORITY_TRAY);
  deactivate();
}

void tray_manager::setup(const string& tray_module_name) {
  const config& conf = config::make();
  auto bs = conf.section();

  // TODO remove check once part of tray module
  if (tray_module_name.empty()) {
    return;
  }

  auto inner_area = m_bar_opts.inner_area();
  unsigned client_height = inner_area.height;

  auto maxsize = conf.get<unsigned>(bs, "tray-maxsize", 16);
  if (client_height > maxsize) {
    m_opts.spacing += (client_height - maxsize) / 2;
    client_height = maxsize;
  }

  // Apply user-defined scaling
  auto scale = conf.get(bs, "tray-scale", 1.0);
  client_height *= scale;

  m_opts.client_size = {client_height, client_height};

  // Set user-defined foreground and background colors.
  // TODO maybe remove
  m_opts.background = conf.get(bs, "tray-background", m_bar_opts.background);
  m_opts.foreground = conf.get(bs, "tray-foreground", m_bar_opts.foreground);

  // Add user-defined padding
  m_opts.spacing += conf.get<unsigned>(bs, "tray-padding", 0);

  m_opts.selection_owner = m_bar_opts.x_data.window;

  // Activate the tray manager
  query_atom();
  activate();
}

bool tray_manager::running() const {
  return m_activated;
}

/**
 * Activate systray management
 */
void tray_manager::activate() {
  if (m_activated) {
    return;
  }

  m_log.info("Activating tray manager");
  m_activated = true;

  m_sig.attach(this);

  try {
    set_tray_colors();
  } catch (const exception& err) {
    m_log.err(err.what());
    m_log.err("Cannot activate tray manager... failed to setup window");
    m_activated = false;
    return;
  }

  // Attempt to get control of the systray selection then
  // notify clients waiting for a manager.
  acquire_selection();

  if (!m_acquired_selection) {
    deactivate();
    return;
  }

  // Send delayed notification
  if (!m_firstactivation) {
    notify_clients();
  } else {
    notify_clients_delayed();
  }

  m_firstactivation = false;
}

/**
 * Deactivate systray management
 */
void tray_manager::deactivate(bool clear_selection) {
  if (!m_activated) {
    return;
  }

  m_log.info("Deactivating tray manager");
  m_activated = false;

  m_sig.detach(this);

  if (!m_connection.connection_has_error() && clear_selection && m_acquired_selection) {
    m_log.trace("tray: Unset selection owner");
    m_connection.set_selection_owner(XCB_NONE, m_atom, XCB_CURRENT_TIME);
  }

  m_log.trace("tray: Unembed clients");
  m_clients.clear();

  m_acquired_selection = false;

  m_connection.flush();

  m_sig.emit(signals::eventqueue::notify_forcechange{});
}

/**
 * Reconfigure tray
 */
void tray_manager::reconfigure() {
  if (!m_opts.selection_owner) {
    return;
  } else if (m_mtx.try_lock()) {
    std::unique_lock<mutex> guard(m_mtx, std::adopt_lock);

    try {
      reconfigure_window();
    } catch (const exception& err) {
      m_log.err("Failed to reconfigure tray window (%s)", err.what());
    }

    try {
      reconfigure_clients();
    } catch (const exception& err) {
      m_log.err("Failed to reconfigure tray clients (%s)", err.what());
    }
    guard.unlock();
    m_connection.flush();
  }

  m_sig.emit(signals::eventqueue::notify_forcechange{});
}

/**
 * Reconfigure container window
 */
void tray_manager::reconfigure_window() {
  m_log.trace("tray: Reconfigure window (hidden=%i, clients=%i)", static_cast<bool>(m_hidden), m_clients.size());
  m_tray_width = calculate_w();
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
      it->reconfigure(x, calculate_client_y());

      x += m_opts.client_size.w + m_opts.spacing;
    } catch (const xpp::x::error::window& err) {
      // TODO print error
      m_log.err("Failed to reconfigure client (%s), removing ... (%s)", m_connection.id(it->client()), err.what());
      remove_client(*it, false);
    }
  }

  m_sig.emit(signals::ui_tray::tray_width_change{m_tray_width});
}

/**
 * Refresh the bar window by clearing it along with each client window
 */
void tray_manager::refresh_window() {
  if (!m_activated || m_hidden || !m_mtx.try_lock()) {
    return;
  }

  std::lock_guard<mutex> lock(m_mtx, std::adopt_lock);

  m_log.trace("tray: Refreshing window");

  for (auto& client : m_clients) {
    try {
      if (client.mapped()) {
        client.clear_window();
      }
    } catch (const std::exception& e) {
      m_log.err("Failed to clear tray client %s '%s' (%s)", ewmh_util::get_wm_name(client.client()),
          m_connection.id(client.client()), e.what());
    }
  }

  m_connection.flush();
}

/**
 * Redraw window
 */
void tray_manager::redraw_window() {
  m_log.info("Redraw tray container");
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
 * Acquire the systray selection
 */
void tray_manager::acquire_selection() {
  m_othermanager = XCB_NONE;
  xcb_window_t owner;

  try {
    owner = m_connection.get_selection_owner(m_atom).owner<xcb_window_t>();
  } catch (const exception& err) {
    return;
  }

  if (owner == m_opts.selection_owner) {
    m_log.trace("tray: Already managing the systray selection");
    m_acquired_selection = true;
  } else if ((m_othermanager = owner) != XCB_NONE) {
    m_log.warn("Systray selection already managed (window=%s)", m_connection.id(owner));
    track_selection_owner(m_othermanager);
  } else {
    m_log.trace("tray: Change selection owner to %s", m_connection.id(m_opts.selection_owner));
    m_connection.set_selection_owner_checked(m_opts.selection_owner, m_atom, XCB_CURRENT_TIME);
    if (m_connection.get_selection_owner_unchecked(m_atom)->owner != m_opts.selection_owner) {
      throw application_error("Failed to get control of the systray selection");
    }
    m_acquired_selection = true;
  }
}

/**
 * Notify pending clients about the new systray MANAGER
 */
void tray_manager::notify_clients() {
  if (m_activated) {
    m_log.info("Notifying pending tray clients");
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
    m_log.trace("tray: Listen for events on the new selection window");
    const unsigned mask{XCB_CW_EVENT_MASK};
    const unsigned values[]{XCB_EVENT_MASK_STRUCTURE_NOTIFY};
    m_connection.change_window_attributes(owner, mask, values);
  }
}

/**
 * Process client docking request
 */
void tray_manager::process_docking_request(xcb_window_t win) {
  m_log.info("Processing docking request from '%s' (%s)", ewmh_util::get_wm_name(win), m_connection.id(win));

  try {
    tray_client client(m_log, m_connection, m_opts.selection_owner, win, m_opts.client_size);

    try {
      client.query_xembed();
    } catch (const xpp::x::error::window& err) {
      m_log.err("Failed to query _XEMBED_INFO, removing client... (%s)", err.what());
      return;
    }

    m_log.trace("tray: xembed = %s", client.is_xembed_supported() ? "true" : "false");
    if (client.is_xembed_supported()) {
      m_log.trace("tray: version = 0x%x, flags = 0x%x, XEMBED_MAPPED = %s", client.get_xembed().get_version(),
          client.get_xembed().get_flags(), client.get_xembed().is_mapped() ? "true" : "false");
    }

    client.update_client_attributes();

    client.reparent();

    client.add_to_save_set();

    client.notify_xembed();

    client.ensure_state();

    m_clients.emplace_back(std::move(client));
  } catch (const std::exception& err) {
    m_log.err("Failed to setup tray client '%s' (%s) removing... (%s)", ewmh_util::get_wm_name(win),
        m_connection.id(win), err.what());
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
 * Check if the given window is embedded
 */
bool tray_manager::is_embedded(const xcb_window_t& win) const {
  return m_clients.end() !=
         std::find_if(m_clients.begin(), m_clients.end(), [win](const auto& client) { return client.match(win); });
}

/**
 * Find tray client by window
 */
tray_client* tray_manager::find_client(const xcb_window_t& win) {
  for (auto& client : m_clients) {
    if (client.match(win)) {
      return &client;
    }
  }
  return nullptr;
}

/**
 * Remove tray client
 */
void tray_manager::remove_client(const tray_client& client, bool reconfigure) {
  remove_client(client.client(), reconfigure);
}

/**
 * Remove tray client by window
 */
void tray_manager::remove_client(xcb_window_t win, bool reconfigure) {
  m_clients.erase(
      std::remove_if(m_clients.begin(), m_clients.end(), [win](const auto& client) { return client.match(win); }));

  if (reconfigure) {
    tray_manager::reconfigure();
  }
}

bool tray_manager::change_visibility(bool visible) {
  if (!m_activated || m_hidden == !visible) {
    return false;
  }

  m_log.trace("tray: visibility_change (state=%i, activated=%i, hidden=%i)", visible, static_cast<bool>(m_activated),
      static_cast<bool>(m_hidden));

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
  if (m_activated && !m_clients.empty() && evt->count == 0) {
    redraw_window();
  }
}

/**
 * Event callback : XCB_VISIBILITY_NOTIFY
 */
void tray_manager::handle(const evt::visibility_notify& evt) {
  if (m_activated && !m_clients.empty()) {
    m_log.trace("tray: Received visibility_notify for %s", m_connection.id(evt->window));
    reconfigure_window();
  }
}

/**
 * Event callback : XCB_CLIENT_MESSAGE
 */
void tray_manager::handle(const evt::client_message& evt) {
  if (!m_activated) {
    return;
  } else if (evt->type == WM_PROTOCOLS && evt->data.data32[0] == WM_DELETE_WINDOW &&
             evt->window == m_opts.selection_owner) {
    m_log.notice("Received WM_DELETE");
    deactivate();
  } else if (evt->type == _NET_SYSTEM_TRAY_OPCODE && evt->format == 32) {
    m_log.trace("tray: Received client_message");

    if (SYSTEM_TRAY_REQUEST_DOCK == evt->data.data32[1]) {
      xcb_window_t win = evt->data.data32[2];
      if (!is_embedded(win)) {
        process_docking_request(win);
      } else {
        m_log.warn("Tray client %s already embedded, ignoring request...", m_connection.id(win));
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
  if (m_activated && is_embedded(evt->window)) {
    try {
      m_log.trace("tray: Client configure request %s", m_connection.id(evt->window));
      find_client(evt->window)->configure_notify();
    } catch (const xpp::x::error::window& err) {
      m_log.err("Failed to reconfigure tray client, removing... (%s)", err.what());
      remove_client(evt->window);
    }
  }
}

/**
 * @see tray_manager::handle(const evt::configure_request&);
 */
void tray_manager::handle(const evt::resize_request& evt) {
  if (m_activated && is_embedded(evt->window)) {
    try {
      m_log.trace("tray: Received resize_request for client %s", m_connection.id(evt->window));
      find_client(evt->window)->configure_notify();
    } catch (const xpp::x::error::window& err) {
      m_log.err("Failed to reconfigure tray client, removing... (%s)", err.what());
      remove_client(evt->window);
    }
  }
}

/**
 * Event callback : XCB_SELECTION_CLEAR
 */
void tray_manager::handle(const evt::selection_clear& evt) {
  if (!m_activated) {
    return;
  } else if (evt->selection != m_atom) {
    return;
  } else if (evt->owner != m_opts.selection_owner) {
    return;
  }

  try {
    m_log.warn("Lost systray selection, deactivating...");
    m_othermanager = m_connection.get_selection_owner(m_atom).owner<xcb_window_t>();
    track_selection_owner(m_othermanager);
  } catch (const exception& err) {
    m_log.err("Failed to get systray selection owner");
    m_othermanager = XCB_NONE;
  }

  deactivate(false);
}

/**
 * Event callback : XCB_PROPERTY_NOTIFY
 */
void tray_manager::handle(const evt::property_notify& evt) {
  if (!m_activated) {
    return;
  }

  if (evt->atom != _XEMBED_INFO) {
    return;
  }

  auto client = find_client(evt->window);

  if (!client) {
    return;
  }

  m_log.trace("tray: _XEMBED_INFO: %s", m_connection.id(evt->window));

  if (evt->state == XCB_PROPERTY_NEW_VALUE) {
    m_log.trace("tray: _XEMBED_INFO value has changed");
  }

  try {
    client->query_xembed();
  } catch (const xpp::x::error::window& err) {
    m_log.err("Failed to query _XEMBED_INFO, removing client... (%s)", err.what());
    remove_client(*client, true);
    return;
  }

  m_log.trace("tray: version = 0x%x, flags = 0x%x, XEMBED_MAPPED = %s", client->get_xembed().get_version(),
      client->get_xembed().get_flags(), client->get_xembed().is_mapped() ? "true" : "false");

  if (client->get_xembed().is_mapped()) {
    reconfigure();
  }
}

/**
 * Event callback : XCB_REPARENT_NOTIFY
 */
void tray_manager::handle(const evt::reparent_notify& evt) {
  if (!m_activated) {
    return;
  }

  auto client = find_client(evt->window);

  if (!client) {
    return;
  }

  if (evt->parent != client->embedder()) {
    m_log.trace("tray: Received reparent_notify for client, remove...");
    remove_client(*client);
  }
}

/**
 * Event callback : XCB_DESTROY_NOTIFY
 */
void tray_manager::handle(const evt::destroy_notify& evt) {
  if (!m_activated && evt->window == m_othermanager) {
    m_log.info("Systray selection unmanaged... re-activating");
    activate();
  } else if (m_activated && is_embedded(evt->window)) {
    m_log.trace("tray: Received destroy_notify for client, remove...");
    remove_client(evt->window);
    redraw_window();
  }
}

/**
 * Event callback : XCB_MAP_NOTIFY
 */
void tray_manager::handle(const evt::map_notify& evt) {
  if (m_activated && evt->window == m_opts.selection_owner) {
    m_log.trace("tray: Received map_notify");
    m_log.trace("tray: Update container mapped flag");
    redraw_window();
  } else if (is_embedded(evt->window)) {
    m_log.trace("tray: Received map_notify");
    m_log.trace("tray: Set client mapped");
    auto client = find_client(evt->window);

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
  if (m_activated && is_embedded(evt->window)) {
    m_log.trace("tray: Received unmap_notify");
    m_log.trace("tray: Set client unmapped");
    auto client = find_client(evt->window);

    if (client->mapped()) {
      client->mapped(false);
      reconfigure();
    }
  }
}

/**
 * Signal handler connected to the bar window's visibility change signal.
 * This is used as a fallback in case the window restacking fails. It will
 * toggle the tray window whenever the visibility of the bar window changes.
 */
bool tray_manager::on(const signals::ui::visibility_change& evt) {
  return change_visibility(evt.cast());
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
