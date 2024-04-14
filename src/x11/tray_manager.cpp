#include "x11/tray_manager.hpp"

#include <xcb/xcb_image.h>

#include <thread>
#include <utility>

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
 * This class manages embedded tray icons by placing them on the bar in the correct positions; the start position is
 * requested by the renderer.
 *
 * The tray manager needs to trigger bar updates only when the size of the entire tray changes (e.g. when tray icons are
 * added/removed). Everything else can be handled without an update.
 */

POLYBAR_NS

namespace tray {

manager::manager(
    connection& conn, signal_emitter& emitter, const logger& logger, const bar_settings& bar_opts, on_update on_update)
    : m_connection(conn), m_sig(emitter), m_log(logger), m_bar_opts(bar_opts), m_on_update(std::move(on_update)) {
  m_connection.attach_sink(this, SINK_PRIORITY_TRAY);
}

manager::~manager() {
  m_connection.detach_sink(this, SINK_PRIORITY_TRAY);
  deactivate();
}

void manager::setup(const config& conf, const string& section_name) {
  unsigned bar_height = m_bar_opts.inner_area().height;

  // Spacing between icons
  auto spacing = conf.get(section_name, "tray-spacing", ZERO_PX_EXTENT);
  m_opts.spacing = units_utils::extent_to_pixel_nonnegative(spacing, m_bar_opts.dpi_x);

  // Padding before and after each icon
  auto padding = conf.get(section_name, "tray-padding", ZERO_PX_EXTENT);
  m_opts.padding = units_utils::extent_to_pixel_nonnegative(padding, m_bar_opts.dpi_x);

  auto size = conf.get(section_name, "tray-size", percentage_with_offset{66., ZERO_PX_EXTENT});
  unsigned client_height = std::min(
      bar_height, units_utils::percentage_with_offset_to_pixel_nonnegative(size, bar_height, m_bar_opts.dpi_y));

  if (client_height == 0) {
    m_log.warn("tray: tray-size has an effective value of 0px, you will not see any tray icons");
  }

  m_opts.client_size = {client_height, client_height};

  // Set user-defined foreground and background colors.
  m_opts.background = conf.get(section_name, "tray-background", m_bar_opts.background);
  m_opts.foreground = conf.get(section_name, "tray-foreground", m_bar_opts.foreground);

  m_opts.selection_owner = m_bar_opts.x_data.window;

  m_log.info("tray: spacing=%upx padding=%upx size=%upx", m_opts.spacing, m_opts.padding, client_height);

  if (m_bar_opts.x_data.window == XCB_NONE) {
    m_log.err("tray: No bar window found, disabling tray");
    return;
  }

  // Activate the tray manager
  query_atom();
  activate();
}

unsigned manager::get_width() const {
  return m_tray_width;
}

bool manager::is_active() const {
  return m_state == state::ACTIVE;
}

bool manager::is_inactive() const {
  return m_state == state::INACTIVE;
}

bool manager::is_waiting() const {
  return m_state == state::WAITING;
}

bool manager::is_visible() const {
  return is_active() && !m_hidden;
}

/**
 * Activate systray management
 */
void manager::activate() {
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

  notify_clients();
}

/**
 * Transitions tray manager to WAITING state
 *
 * @param other window id for current selection owner
 */
void manager::wait_for_selection(xcb_window_t other) {
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

  recalculate_width();
}

/**
 * Deactivate systray management
 */
void manager::deactivate() {
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

  recalculate_width();
}

/**
 * Reconfigure tray
 */
void manager::reconfigure() {
  if (!m_opts.selection_owner) {
    return;
  }

  try {
    reconfigure_clients();
  } catch (const exception& err) {
    m_log.err("Failed to reconfigure tray clients (%s)", err.what());
  }

  m_connection.flush();
}

/**
 * Calculates the total width of the tray and potentially runs the update hook.
 *
 * Should be called whenever the number of mapped clients changes
 */
void manager::recalculate_width() {
  m_log.trace("tray: Reconfigure window (hidden=%i, clients=%i)", m_hidden, m_clients.size());
  unsigned new_width = calculate_w();
  if (m_tray_width != new_width) {
    m_tray_width = new_width;
    m_log.trace("tray: new width (width: %d, clients: %d)", m_tray_width, m_clients.size());
    m_on_update();
  }
}

/**
 * Reconfigure client positions and mapped state
 */
void manager::reconfigure_clients() {
  m_log.trace("tray: Reconfigure clients");

  // X-position of the start of the tray area
  int base_x = calculate_x();

  // X-position of the end of the previous tray icon (including padding)
  unsigned x = 0;

  bool has_error = false;

  unsigned count = 0;

  for (auto& client : m_clients) {
    try {
      client->ensure_state();

      if (client->mapped()) {
        // Calculate start of tray icon
        unsigned client_x = x + (count > 0 ? m_opts.spacing : 0) + m_opts.padding;
        client->set_position(base_x + client_x, calculate_client_y());
        // Add size and padding to get the end position of the icon
        x = client_x + m_opts.client_size.w + m_opts.padding;
        count++;
      }
    } catch (const xpp::x::error::window& err) {
      m_log.err("Failed to reconfigure %s, removing ... (%s)", client->name(), err.what());
      client.reset();
      has_error = true;
    }
  }

  if (has_error) {
    clean_clients();
  }

  // Some clients may have been (un)mapped or even removed
  recalculate_width();
  // The final x position should match the width of the entire tray
  assert(x == m_tray_width);
}

/**
 * Redraw client windows.
 */
void manager::redraw_clients() {
  if (!is_visible()) {
    return;
  }

  m_log.trace("tray: Refreshing clients");

  for (auto& client : m_clients) {
    try {
      if (client->mapped()) {
        client->update_bg();
      }
    } catch (const std::exception& e) {
      m_log.err("tray: Failed to clear %s (%s)", client->name(), e.what());
    }
  }

  m_connection.flush();
}

/**
 * Find the systray selection atom
 */
void manager::query_atom() {
  m_log.trace("tray: Find systray selection atom for the default screen");
  string name{"_NET_SYSTEM_TRAY_S" + to_string(m_connection.default_screen())};
  auto reply = m_connection.intern_atom(false, name.length(), name.c_str());
  m_atom = reply.atom();
}

/**
 * Set _NET_SYSTEM_TRAY_COLORS atom used by clients when determing icon theme
 */
void manager::set_tray_colors() {
  m_log.trace("tray: Set _NET_SYSTEM_TRAY_COLORS to 0x%08x", m_opts.foreground);

  auto r = m_opts.foreground.red_i();
  auto g = m_opts.foreground.green_i();
  auto b = m_opts.foreground.blue_i();

  const uint16_t r16 = (r << 8) | r;
  const uint16_t g16 = (g << 8) | g;
  const uint16_t b16 = (b << 8) | b;

  const array<uint32_t, 12> colors = {
      r16, g16, b16, // normal
      r16, g16, b16, // error
      r16, g16, b16, // warning
      r16, g16, b16, // success
  };

  m_connection.change_property(XCB_PROP_MODE_REPLACE, m_opts.selection_owner, _NET_SYSTEM_TRAY_COLORS,
      XCB_ATOM_CARDINAL, 32, colors.size(), colors.data());
}

/**
 * Set the _NET_SYSTEM_TRAY_ORIENTATION atom
 */
void manager::set_tray_orientation() {
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
bool manager::acquire_selection(xcb_window_t& other_owner) {
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
void manager::notify_clients() {
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
 * Track changes to the given selection owner
 * If it gets destroyed or goes away we can reactivate the tray_manager
 */
void manager::track_selection_owner(xcb_window_t owner) {
  if (owner != XCB_NONE) {
    const uint32_t mask{XCB_CW_EVENT_MASK};
    const uint32_t value{XCB_EVENT_MASK_STRUCTURE_NOTIFY};
    m_connection.change_window_attributes(owner, mask, &value);
  }
}

/**
 * Process client docking request
 */
void manager::process_docking_request(xcb_window_t win) {
  m_log.info("tray: Processing docking request from '%s' (%s)", ewmh_util::get_wm_name(win), m_connection.id(win));

  try {
    auto cl =
        make_unique<client>(m_log, m_connection, m_opts.selection_owner, win, m_opts.client_size, m_opts.background);

    try {
      cl->query_xembed();
    } catch (const xpp::x::error::window& err) {
      m_log.err("Failed to query _XEMBED_INFO, removing %s ... (%s)", cl->name(), err.what());
      return;
    }

    cl->update_client_attributes();

    cl->reparent();

    cl->add_to_save_set();

    cl->hidden(m_hidden);
    cl->ensure_state();

    cl->notify_xembed();

    m_clients.emplace_back(std::move(cl));
  } catch (const std::exception& err) {
    m_log.err("tray: Failed to setup tray client '%s' (%s) removing... (%s)", ewmh_util::get_wm_name(win),
        m_connection.id(win), err.what());
    return;
  }
}

/**
 * Final x-position of the tray window relative to the very top-left bar window.
 */
int manager::calculate_x() const {
  return m_bar_opts.inner_area(false).x + m_pos.x;
}

/**
 * Calculates the entire width taken up by the tray area in pixels
 *
 * This many pixels need to be reserved on the bar in order to draw the tray.
 */
unsigned manager::calculate_w() const {
  unsigned count =
      std::count_if(m_clients.begin(), m_clients.end(), [](const auto& client) { return client->mapped(); });

  if (count > 0) {
    return (count - 1) * m_opts.spacing + count * (2 * m_opts.padding + m_opts.client_size.w);
  } else {
    return 0;
  }
}

/**
 * Calculate y position of client window to vertically center it in the inner area of the bar.
 */
int manager::calculate_client_y() {
  return m_bar_opts.inner_area(false).y + (m_bar_opts.inner_area(false).height - m_opts.client_size.h) / 2;
}

/**
 * Check if the given window is embedded.
 *
 * The given window ID can be the ID of the wrapper or the embedded window
 */
bool manager::is_embedded(const xcb_window_t& win) {
  return find_client(win) != nullptr;
}

/**
 * Find tray client object from the wrapper or embedded window
 */
client* manager::find_client(const xcb_window_t& win) {
  auto client = std::find_if(m_clients.begin(), m_clients.end(),
      [win](const auto& client) { return client->match(win) || client->embedder() == win; });

  if (client == m_clients.end()) {
    return nullptr;
  } else {
    return client->get();
  }
}

/**
 * Remove tray client
 */
void manager::remove_client(const client& c) {
  remove_client(c.client_window());
}

/**
 * Remove tray client by window
 */
void manager::remove_client(xcb_window_t win) {
  auto old_size = m_clients.size();
  m_clients.erase(
      std::remove_if(m_clients.begin(), m_clients.end(), [win](const auto& client) { return client->match(win); }),
      m_clients.end());

  if (old_size != m_clients.size()) {
    reconfigure();
  }
}

/**
 * Remove all null pointers from client list.
 *
 * Removing clients is often done in two steps:
 * 1. When removing a client during iteration, the unique_ptr is reset.
 * 2. Afterwards all null pointers are removed from the list.
 */
void manager::clean_clients() {
  m_clients.erase(
      std::remove_if(m_clients.begin(), m_clients.end(), [](const auto& client) { return client.get() == nullptr; }),
      m_clients.end());
}

bool manager::change_visibility(bool visible) {
  if (m_hidden == !visible) {
    return false;
  }

  m_log.trace("tray: visibility_change (new_state)", visible ? "visible" : "hidden");

  m_hidden = !visible;

  for (auto& client : m_clients) {
    client->hidden(m_hidden);
    client->ensure_state();
  }

  if (!m_hidden) {
    redraw_clients();
  }

  m_connection.flush();

  return true;
}

/**
 * Event callback : XCB_EXPOSE
 */
void manager::handle(const evt::expose& evt) {
  if (is_active() && !m_clients.empty() && evt->count == 0) {
    redraw_clients();
  }
}

/**
 * Event callback : XCB_CLIENT_MESSAGE
 */
void manager::handle(const evt::client_message& evt) {
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
void manager::handle(const evt::configure_request& evt) {
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
 * @see manager::handle(const evt::configure_request&);
 */
void manager::handle(const evt::resize_request& evt) {
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
void manager::handle(const evt::selection_clear& evt) {
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
void manager::handle(const evt::property_notify& evt) {
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
void manager::handle(const evt::reparent_notify& evt) {
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
void manager::handle(const evt::destroy_notify& evt) {
  if (is_waiting() && evt->window == m_othermanager) {
    m_log.info("Systray selection unmanaged... re-activating");
    activate();
  } else if (is_active() && is_embedded(evt->window)) {
    m_log.info("tray: Received destroy_notify for client, remove...");
    remove_client(evt->window);
    redraw_clients();
  }
}

/**
 * Event callback : XCB_MAP_NOTIFY
 */
void manager::handle(const evt::map_notify& evt) {
  if (is_active() && evt->window == m_opts.selection_owner) {
    m_log.trace("tray: Received map_notify for selection owner");
    redraw_clients();
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
void manager::handle(const evt::unmap_notify& evt) {
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

bool manager::on(const signals::ui::update_background&) {
  redraw_clients();

  return false;
}

bool manager::on(const signals::ui_tray::tray_pos_change& evt) {
  int new_x = std::max(0, std::min(evt.cast(), (int)(m_bar_opts.size.w - m_tray_width)));

  if (new_x != m_pos.x) {
    m_pos.x = new_x;
    reconfigure();
  }

  return true;
}

} // namespace tray

POLYBAR_NS_END
