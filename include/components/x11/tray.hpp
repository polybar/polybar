#pragma once

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

#include "common.hpp"
#include "components/logger.hpp"
#include "components/signals.hpp"
#include "components/types.hpp"
#include "components/x11/connection.hpp"
#include "components/x11/xembed.hpp"
#include "utils/memory.hpp"
#include "utils/process.hpp"

#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define _NET_SYSTEM_TRAY_ORIENTATION_VERT 1

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

#define TRAY_WM_NAME "Lemonbuddy tray window"
#define TRAY_WM_CLASS "tray\0Lemonbuddy"

LEMONBUDDY_NS

// class definition : trayclient {{{

class trayclient {
 public:
  explicit trayclient(connection& conn, xcb_window_t win) : m_connection(conn), m_window(win) {
    m_xembed = memory_util::make_malloc_ptr<xembed_data>();
    m_xembed->version = XEMBED_VERSION;
    m_xembed->flags = XEMBED_MAPPED;
  }

  ~trayclient() {
    xembed::unembed(m_connection, window(), m_connection.root());
  }

  /**
   * Match given window against client window
   */
  bool match(const xcb_window_t& win) const {
    return win == m_window;
  }

  /**
   * Get client window mapped state
   */
  bool mapped() const {
    return m_mapped;
  }

  /**
   * Set client window mapped state
   */
  void mapped(bool state) {
    m_mapped = state;
  }

  /**
   * Get client window
   */
  xcb_window_t window() const {
    return m_window;
  }

  /**
   * Get xembed data pointer
   */
  xembed_data* xembed() const {
    return m_xembed.get();
  }

  void configure_notify(int16_t x, int16_t y, uint16_t w, uint16_t h) {
    auto notify = reinterpret_cast<xcb_configure_notify_event_t*>(calloc(32, 1));

    notify->response_type = XCB_CONFIGURE_NOTIFY;
    notify->event = m_window;
    notify->window = m_window;
    notify->override_redirect = false;
    notify->above_sibling = XCB_NONE;
    notify->x = x;
    notify->y = y;
    notify->width = w;
    notify->height = h;
    notify->border_width = 0;

    m_connection.send_event(
        false, notify->event, XCB_EVENT_MASK_STRUCTURE_NOTIFY, reinterpret_cast<char*>(notify));
    m_connection.flush();

    free(notify);
  }

 protected:
  connection& m_connection;
  xcb_window_t m_window{0};
  shared_ptr<xembed_data> m_xembed;
  stateflag m_mapped{false};
};

// }}}
// class definition : traymanager {{{

class traymanager
    : public xpp::event::sink<evt::expose, evt::visibility_notify, evt::client_message,
          evt::configure_request, evt::resize_request, evt::selection_clear, evt::property_notify,
          evt::reparent_notify, evt::destroy_notify, evt::map_notify, evt::unmap_notify> {
 public:
  explicit traymanager(connection& conn, const logger& logger) : m_connection(conn), m_log(logger) {
    m_connection.attach_sink(this, 2);
    m_sinkattached = true;
  }

  ~traymanager() {
    if (m_activated)
      deactivate();
    if (m_sinkattached)
      m_connection.detach_sink(this, 2);
  }

  /**
   * Initialize data
   */
  auto bootstrap(tray_settings settings) {
    m_settings = settings;
    query_atom();
  }

  /**
   * Activate systray management
   */
  void activate() {
    if (m_activated) {
      return;
    }

    if (m_tray == XCB_NONE) {
      try {
        create_window();
        set_wmhints();
      } catch (const std::exception& err) {
        m_log.err(err.what());
        m_log.err("Cannot activate traymanager... failed to setup window");
        return;
      }
    }

    m_log.info("Activating traymanager");
    m_activated = true;

    if (!m_sinkattached) {
      m_connection.attach_sink(this, 2);
      m_sinkattached = true;
    }

    // Listen for visibility change events on the bar window
    if (!m_restacked && !g_signals::bar::visibility_change) {
      g_signals::bar::visibility_change =
          bind(&traymanager::bar_visibility_change, this, std::placeholders::_1);
    }

    // Attempt to get control of the systray selection then
    // notify clients waiting for a manager.
    acquire_selection();

    // If replacing an existing manager or if re-activating from getting
    // replaced, we delay the notification broadcast to allow the clients
    // to get unembedded...
    if (m_othermanager != XCB_NONE)
      this_thread::sleep_for(1s);
    notify_clients();

    m_connection.flush();
  }

  /**
   * Deactivate systray management
   */
  void deactivate() {
    if (!m_activated) {
      return;
    }

    m_log.info("Deactivating traymanager");
    m_activated = false;

    if (m_delayed_activation.joinable())
      m_delayed_activation.join();

    if (g_signals::tray::report_slotcount) {
      m_log.trace("tray: Report empty slotcount");
      g_signals::tray::report_slotcount(0);
    }

    if (g_signals::bar::visibility_change) {
      m_log.trace("tray: Clear callback handlers");
      g_signals::bar::visibility_change = nullptr;
    }

    m_log.trace("tray: Unembed all clients");
    m_clients.clear();

    if (m_connection.get_selection_owner_unchecked(m_atom).owner<xcb_window_t>() == m_tray) {
      m_log.trace("tray: Unset selection owner");
      m_connection.set_selection_owner(XCB_NONE, m_atom, XCB_CURRENT_TIME);
    }

    if (m_tray != XCB_NONE) {
      if (m_mapped) {
        m_log.trace("tray: Unmap window");
        m_connection.unmap_window(m_tray);
        m_mapped = false;
      }

      m_log.trace("tray: Destroy window");
      m_connection.destroy_window(m_tray);
      m_tray = XCB_NONE;
      m_hidden = false;
    }

    m_connection.flush();
  }

  /**
   * Reconfigure container window size and
   * reposition embedded clients
   */
  void reconfigure() {
    // Ignore reconfigure requests when the
    // tray window is in the pseudo-hidden state
    if (m_hidden)
      return;
    if (m_tray == XCB_NONE)
      return;

    uint32_t width = 0;
    uint16_t mapped_clients = 0;

    for (auto&& client : m_clients) {
      if (client->mapped()) {
        mapped_clients++;
        width += m_settings.spacing + m_settings.width;
      }
    }

    if (g_signals::tray::report_slotcount)
      g_signals::tray::report_slotcount(mapped_clients);

    if (!width && m_mapped) {
      m_connection.unmap_window_checked(m_tray);
      return;
    } else if (width && !m_mapped) {
      m_connection.map_window_checked(m_tray);
      m_lastwidth = 0;
      return;
    } else if (!width) {
      return;
    }

    if ((width += m_settings.spacing) == m_lastwidth) {
      return;
    }

    m_lastwidth = width;

    // update window
    const uint32_t val[2]{static_cast<uint32_t>(calculate_x()), width};
    m_connection.configure_window(m_tray, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH, val);

    // reposition clients
    uint32_t pos_x = m_settings.spacing;
    for (auto&& client : m_clients) {
      if (!client->mapped())
        continue;
      try {
        const uint32_t val[1]{pos_x};
        m_connection.configure_window_checked(client->window(), XCB_CONFIG_WINDOW_X, val);
        pos_x += m_settings.width + m_settings.spacing;
      } catch (const xpp::x::error::window& err) {
        m_log.warn("Failed to reconfigure tray client, removing... (%s)", err.what());
        m_clients.erase(std::find(m_clients.begin(), m_clients.end(), client));
        reconfigure();
        return;
      }
    }
  }

 protected:
  /**
   * Signal handler connected to the bar window's visibility change signal.
   * This is used as a fallback in case the window restacking fails. It will
   * toggle the tray window whenever the visibility of the bar window changes.
   */
  void bar_visibility_change(bool state) {
    try {
      // Ignore unchanged states
      if (m_hidden == !state)
        return;

      // Update the psuedo-state
      m_hidden = !state;

      if (state && !m_mapped)
        m_connection.map_window_checked(m_tray);
      else if (!state && m_mapped)
        m_connection.unmap_window_checked(m_tray);
    } catch (const std::exception& err) {
      m_log.warn("Failed to un-/map the tray window (%s)", err.what());
    }
  }

  /**
   * Calculate the tray window's horizontal position
   */
  int16_t calculate_x() const {
    auto x = m_settings.orig_x;
    if (m_settings.align == alignment::RIGHT)
      x -= ((m_settings.width + m_settings.spacing) * m_clients.size() + m_settings.spacing);
    return x;
  }

  /**
   * Calculate the tray window's vertical position
   */
  int16_t calculate_y() const {
    return m_settings.orig_y;
  }

  /**
   * Find tray client by window
   */
  shared_ptr<trayclient> find_client(const xcb_window_t& win) {
    for (auto&& client : m_clients)
      if (client->match(win)) {
        return shared_ptr<trayclient>{client.get(), null_deleter{}};
      }
    return {};
  }

  /**
   * Find the systray selection atom
   */
  void query_atom() {
    m_log.trace("tray: Find systray selection atom for the default screen");
    string name{"_NET_SYSTEM_TRAY_S" + to_string(m_connection.default_screen())};
    auto reply = m_connection.intern_atom(false, name.length(), name.c_str());
    m_atom = reply.atom();
  }

  /**
   * Create tray window
   */
  void create_window() {
    auto x = calculate_x();
    auto y = calculate_y();
    m_tray = m_connection.generate_id();
    m_log.trace("tray: Create tray window %s, (%ix%i+%i+%i)", m_connection.id(m_tray),
        m_settings.width, m_settings.height, x, y);
    auto scr = m_connection.screen();
    const uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
    const uint32_t values[3]{m_settings.background, true,
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY};
    m_connection.create_window_checked(scr->root_depth, m_tray, scr->root, x, y,
        m_settings.width + m_settings.spacing * 2, m_settings.height + m_settings.spacing * 2, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT, scr->root_visual, mask, values);

    try {
      // Put the tray window above the defined sibling in the window stack
      if (m_settings.sibling != XCB_NONE) {
        const uint32_t value_mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
        const uint32_t value_list[2]{m_settings.sibling, XCB_STACK_MODE_ABOVE};
        m_connection.configure_window_checked(m_tray, value_mask, value_list);
        m_connection.flush();
        m_restacked = true;
      }
    } catch (const std::exception& err) {
      auto id = m_connection.id(m_settings.sibling);
      m_log.trace("tray: Failed to put tray above %s in the stack (%s)", id, err.what());
    }
  }

  /**
   * Set window WM hints
   */
  void set_wmhints() {
    m_log.trace("tray: Set window WM_NAME / WM_CLASS", m_connection.id(m_tray));
    xcb_icccm_set_wm_name(m_connection, m_tray, XCB_ATOM_STRING, 8, 22, TRAY_WM_NAME);
    xcb_icccm_set_wm_class(m_connection, m_tray, 15, TRAY_WM_CLASS);

    m_log.trace("tray: Set window WM_PROTOCOLS");
    vector<xcb_atom_t> wm_flags;
    wm_flags.emplace_back(WM_DELETE_WINDOW);
    wm_flags.emplace_back(WM_TAKE_FOCUS);
    xcb_icccm_set_wm_protocols(
        m_connection, m_tray, WM_PROTOCOLS, wm_flags.size(), wm_flags.data());

    m_log.trace("tray: Set window _NET_WM_WINDOW_TYPE");
    vector<xcb_atom_t> types;
    types.emplace_back(_NET_WM_WINDOW_TYPE_DOCK);
    types.emplace_back(_NET_WM_WINDOW_TYPE_NORMAL);
    m_connection.change_property(XCB_PROP_MODE_REPLACE, m_tray, _NET_WM_WINDOW_TYPE, XCB_ATOM_ATOM,
        32, types.size(), types.data());

    m_log.trace("tray: Set window _NET_WM_STATE");
    vector<xcb_atom_t> states;
    states.emplace_back(_NET_WM_STATE_SKIP_TASKBAR);
    m_connection.change_property(XCB_PROP_MODE_REPLACE, m_tray, _NET_WM_STATE, XCB_ATOM_ATOM, 32,
        states.size(), states.data());

    m_log.trace("tray: Set window _NET_SYSTEM_TRAY_ORIENTATION");
    const uint32_t values[1]{_NET_SYSTEM_TRAY_ORIENTATION_HORZ};
    m_connection.change_property_checked(XCB_PROP_MODE_REPLACE, m_tray,
        _NET_SYSTEM_TRAY_ORIENTATION, _NET_SYSTEM_TRAY_ORIENTATION, 32, 1, values);

    m_log.trace("tray: Set window _NET_WM_PID");
    int pid = getpid();
    m_connection.change_property(
        XCB_PROP_MODE_REPLACE, m_tray, _NET_WM_PID, XCB_ATOM_CARDINAL, 32, 1, &pid);
  }

  /**
   * Acquire the systray selection
   */
  void acquire_selection() {
    xcb_window_t owner = m_connection.get_selection_owner_unchecked(m_atom)->owner;

    if (owner == m_tray) {
      m_log.info("tray: Already managing the systray selection");
      return;
    } else if ((m_othermanager = owner) != XCB_NONE) {
      m_log.info("Replacing selection manager %s", m_connection.id(owner));
    }

    m_log.trace("tray: Change selection owner to %s", m_connection.id(m_tray));
    m_connection.set_selection_owner_checked(m_tray, m_atom, XCB_CURRENT_TIME);

    if (m_connection.get_selection_owner_unchecked(m_atom)->owner != m_tray)
      throw application_error("Failed to get control of the systray selection");
  }

  /**
   * Notify pending clients about the new systray MANAGER
   */
  void notify_clients() {
    m_log.trace("tray: Broadcast new selection manager to pending clients");
    auto message = m_connection.make_client_message(MANAGER, m_connection.root());
    message->data.data32[0] = XCB_CURRENT_TIME;
    message->data.data32[1] = m_atom;
    message->data.data32[2] = m_tray;
    m_connection.send_client_message(message, m_connection.root());
  }

  /**
   * Track changes to the given selection owner
   * If it gets destroyed or goes away we can reactivate the traymanager
   */
  void track_selection_owner(xcb_window_t owner) {
    try {
      if (owner == XCB_NONE)
        return;
      m_log.trace("tray: Listen for events on the new selection window");
      const uint32_t event_mask[1]{XCB_EVENT_MASK_STRUCTURE_NOTIFY};
      m_connection.change_window_attributes_checked(owner, XCB_CW_EVENT_MASK, event_mask);
    } catch (const xpp::x::error::window& err) {
      m_log.err("Failed to track selection owner");
    }
  }

  /**
   * Calculates a client's x position
   */
  int calculate_client_xpos(const xcb_window_t& win) {
    for (size_t i = 0; i < m_clients.size(); i++)
      if (m_clients[i]->match(win))
        return m_settings.spacing + m_settings.width * i;
    return m_settings.spacing;
  }

  /**
   * Process client docking request
   */
  void process_docking_request(xcb_window_t win) {
    auto client = find_client(win);
    if (client) {
      if (client->mapped()) {
        m_log.trace("tray: Client %s is already embedded, skipping...", m_connection.id(win));
      } else {
        m_log.trace("tray: Refresh _XEMBED_INFO");
        xembed::query(m_connection, win, client->xembed());

        if ((client->xembed()->flags & XEMBED_MAPPED) == XEMBED_MAPPED) {
          m_log.trace("tray: XEMBED_MAPPED flag set, map client window...");
          try {
            m_connection.map_window_checked(client->window());
          } catch (const xpp::x::error::window& err) {
            m_log.warn("Failed to map tray client, removing... (%s)", err.what());
            m_clients.erase(std::find(m_clients.begin(), m_clients.end(), client));
            return;
          }
        }
      }

      return;
    }

    m_log.trace("tray: Process docking request from %s", m_connection.id(win));
    m_clients.emplace_back(make_shared<trayclient>(m_connection, win));
    client = m_clients.back();

    try {
      m_log.trace("tray: Get client _XEMBED_INFO");
      xembed::query(m_connection, win, client->xembed());
    } catch (const application_error& err) {
      m_log.err(err.what());
    } catch (const xpp::x::error::window& err) {
      m_log.warn("Failed to query for _XEMBED_INFO, removing client... (%s)", err.what());
      m_clients.erase(std::find(m_clients.begin(), m_clients.end(), client));
      return;
    }

    try {
      m_log.trace("tray: Add tray client window to the save set");
      m_connection.change_save_set_checked(XCB_SET_MODE_INSERT, client->window());

      m_log.trace("tray: Update tray client event mask");
      const uint32_t event_mask[]{XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY};
      m_connection.change_window_attributes_checked(client->window(), XCB_CW_EVENT_MASK, event_mask);

      m_log.trace("tray: Reparent tray client");
      m_connection.reparent_window_checked(client->window(), m_tray, m_settings.spacing, m_settings.spacing);

      m_log.trace("tray: Configure tray client size");
      const uint32_t values[]{m_settings.width, m_settings.height};
      m_connection.configure_window_checked(
          client->window(), XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);

      m_log.trace("tray: Send embbeded notification to tray client");
      xembed::notify_embedded(m_connection, client->window(), m_tray, client->xembed()->version);

      m_log.trace("tray: Map tray client");
      m_connection.map_window_checked(client->window());
    } catch (const xpp::x::error::window& err) {
      m_log.warn("Failed to map client, removing... (%s)", err.what());
      m_clients.erase(std::find(m_clients.begin(), m_clients.end(), client));
    }
  }

  /**
   * Event callback : XCB_EXPOSE
   */
  void handle(const evt::expose& evt) {
    if (!m_activated)
      return;
    if (m_clients.empty())
      return;
    m_log.trace("tray: Received expose event for %s", m_connection.id(evt->window));
    reconfigure();
  }

  /**
   * Event callback : XCB_VISIBILITY_NOTIFY
   */
  void handle(const evt::visibility_notify& evt) {
    if (!m_activated || m_clients.empty())
      return;
    if (m_clients.empty())
      return;
    m_log.trace("tray: Received visibility_notify for %s", m_connection.id(evt->window));
    reconfigure();
  }

  /**
   * Event callback : XCB_CLIENT_MESSAGE
   */
  void handle(const evt::client_message& evt) {
    if (!m_activated)
      return;

    if (evt->type == _NET_SYSTEM_TRAY_OPCODE && evt->format == 32) {
      m_log.trace("tray: Received client_message");

      switch (evt->data.data32[1]) {
        case SYSTEM_TRAY_REQUEST_DOCK:
          try {
            process_docking_request(evt->data.data32[2]);
          } catch (const std::exception& err) {
            auto id = m_connection.id(evt->data.data32[2]);
            m_log.err("Error while processing docking request for %s (%s)", id, err.what());
          }
          return;

        case SYSTEM_TRAY_BEGIN_MESSAGE:
          // process_messages(...);
          return;

        case SYSTEM_TRAY_CANCEL_MESSAGE:
          // process_messages(...);
          return;
      }
    } else if (evt->type == WM_PROTOCOLS && evt->data.data32[0] == WM_DELETE_WINDOW) {
      if (evt->window == m_tray) {
        m_log.warn("Received WM_DELETE");
        m_tray = XCB_NONE;
        deactivate();
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
  void handle(const evt::configure_request& evt) {
    if (!m_activated)
      return;

    auto client = find_client(evt->window);
    if (client) {
      m_log.trace("tray: Received configure_request for client %s", m_connection.id(evt->window));
      client->configure_notify(calculate_client_xpos(evt->window), m_settings.spacing,
          m_settings.width, m_settings.height);
    }
  }

  /**
   * @see tray_manager::handle(const evt::configure_request&);
   */
  void handle(const evt::resize_request& evt) {
    if (!m_activated)
      return;

    auto client = find_client(evt->window);
    if (client) {
      m_log.trace("tray: Received resize_request for client %s", m_connection.id(evt->window));
      client->configure_notify(calculate_client_xpos(evt->window), m_settings.spacing,
          m_settings.width, m_settings.height);
    }
  }

  /**
   * Event callback : XCB_SELECTION_CLEAR
   */
  void handle(const evt::selection_clear& evt) {
    if (!m_activated)
      return;
    if (evt->selection != m_atom)
      return;
    if (evt->owner != m_tray)
      return;

    try {
      m_log.warn("Lost systray selection, deactivating...");
      m_othermanager = m_connection.get_selection_owner(m_atom)->owner;
      track_selection_owner(m_othermanager);
    } catch (const std::exception& err) {
      m_log.err("Failed to get systray selection owner");
      m_othermanager = XCB_NONE;
    }

    deactivate();
  }

  /**
   * Event callback : XCB_PROPERTY_NOTIFY
   */
  void handle(const evt::property_notify& evt) {
    if (!m_activated)
      return;
    if (evt->atom != _XEMBED_INFO)
      return;

    m_log.trace("tray: _XEMBED_INFO: %s", m_connection.id(evt->window));

    auto client = find_client(evt->window);
    if (!client)
      return;

    auto xd = client->xembed();
    auto win = client->window();

    if (evt->state == XCB_PROPERTY_NEW_VALUE)
      m_log.trace("tray: _XEMBED_INFO value has changed");

    xembed::query(m_connection, win, xd);
    m_log.trace("tray: _XEMBED_INFO[0]=%u _XEMBED_INFO[1]=%u", xd->version, xd->flags);

    try {
      if (!client->mapped() && ((xd->flags & XEMBED_MAPPED) == XEMBED_MAPPED)) {
        m_log.info("tray: Map client window: %s", m_connection.id(win));
        m_connection.map_window(win);
      } else if (client->mapped() && ((xd->flags & XEMBED_MAPPED) != XEMBED_MAPPED)) {
        m_log.info("tray: Unmap client window: %s", m_connection.id(win));
        m_connection.unmap_window(win);
      }
    } catch (const xpp::x::error::window& err) {
      m_log.warn("Failed to map/unmap client, removing... (%s)", err.what());
      m_clients.erase(std::find(m_clients.begin(), m_clients.end(), client));
    }
  }

  /**
   * Event callback : XCB_REPARENT_NOTIFY
   */
  void handle(const evt::reparent_notify& evt) {
    if (!m_activated)
      return;
    if (evt->parent == m_tray)
      return;

    auto client = find_client(evt->window);

    if (client) {
      m_log.trace("tray: Received reparent_notify");
      m_log.trace("tray: Remove tray client");
      m_clients.erase(std::find(m_clients.begin(), m_clients.end(), client));
      reconfigure();
    }
  }

  /**
   * Event callback : XCB_DESTROY_NOTIFY
   */
  void handle(const evt::destroy_notify& evt) {
    if (!m_activated && evt->window == m_othermanager) {
      m_log.trace("tray: Received destroy_notify");
      m_log.trace("tray: Systray selection is available... re-activating");
      activate();
    } else if (m_activated) {
      auto client = find_client(evt->window);

      if (client) {
        m_log.trace("tray: Received destroy_notify");
        m_log.trace("tray: Remove tray client");
        m_clients.erase(std::find(m_clients.begin(), m_clients.end(), client));
        reconfigure();
      }
    }
  }

  /**
   * Event callback : XCB_MAP_NOTIFY
   */
  void handle(const evt::map_notify& evt) {
    if (!m_activated)
      return;

    if (evt->window == m_tray && !m_mapped) {
      m_log.trace("tray: Received map_notify");
      if (m_mapped)
        return;
      m_log.trace("tray: Update container mapped flag");
      m_mapped = true;
      reconfigure();
    } else {
      auto client = find_client(evt->window);
      if (client) {
        m_log.trace("tray: Received map_notify");
        m_log.trace("tray: Set client mapped");
        client->mapped(true);
        reconfigure();
      }
    }
  }

  /**
   * Event callback : XCB_UNMAP_NOTIFY
   */
  void handle(const evt::unmap_notify& evt) {
    if (!m_activated)
      return;

    if (evt->window == m_tray) {
      m_log.trace("tray: Received unmap_notify");
      if (!m_mapped)
        return;
      m_log.trace("tray: Update container mapped flag");
      m_mapped = false;
      reconfigure();
    } else {
      auto client = find_client(evt->window);

      if (client) {
        m_log.trace("tray: Received unmap_notify");
        m_log.trace("tray: Set client unmapped");
        client->mapped(true);
        reconfigure();
      }
    }
  }

 private:
  connection& m_connection;
  const logger& m_log;
  vector<shared_ptr<trayclient>> m_clients;

  tray_settings m_settings;

  xcb_atom_t m_atom{0};
  xcb_window_t m_tray{0};
  xcb_window_t m_othermanager{0};
  uint32_t m_lastwidth;

  stateflag m_activated{false};
  stateflag m_mapped{false};
  stateflag m_hidden{false};
  stateflag m_sinkattached{false};

  thread m_delayed_activation;

  bool m_restacked = false;
};

// }}}

namespace {
  /**
   * Configure injection module
   */
  template <class T = unique_ptr<traymanager>>
  di::injector<T> configure_traymanager() {
    return di::make_injector(configure_logger(), configure_connection());
  }
}

LEMONBUDDY_NS_END
