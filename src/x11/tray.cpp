#include <xcb/xcb_icccm.h>
#include <xcb/xcb_image.h>
#include <chrono>
#include <thread>

#include "components/signals.hpp"
#include "components/types.hpp"
#include "utils/color.hpp"
#include "utils/factory.hpp"
#include "utils/memory.hpp"
#include "utils/process.hpp"
#include "x11/color.hpp"
#include "x11/connection.hpp"
#include "x11/draw.hpp"
#include "x11/events.hpp"
#include "x11/graphics.hpp"
#include "x11/tray.hpp"
#include "x11/window.hpp"
#include "x11/wm.hpp"
#include "x11/xembed.hpp"

POLYBAR_NS

// implementation : tray_client {{{

tray_client::tray_client(connection& conn, xcb_window_t win, uint16_t w, uint16_t h)
    : m_connection(conn), m_window(win), m_width(w), m_height(h) {
  m_xembed = memory_util::make_malloc_ptr<xembed_data>();
  m_xembed->version = XEMBED_VERSION;
  m_xembed->flags = XEMBED_MAPPED;
}

tray_client::~tray_client() {
  xembed::unembed(m_connection, window(), m_connection.root());
}

/**
 * Match given window against client window
 */
bool tray_client::match(const xcb_window_t& win) const {  // {{{
  return win == m_window;
}  // }}}

/**
 * Get client window mapped state
 */
bool tray_client::mapped() const {  // {{{
  return m_mapped;
}  // }}}

/**
 * Set client window mapped state
 */
void tray_client::mapped(bool state) {  // {{{
  m_mapped = state;
}  // }}}

/**
 * Get client window
 */
xcb_window_t tray_client::window() const {  // {{{
  return m_window;
}  // }}}

/**
 * Get xembed data pointer
 */
xembed_data* tray_client::xembed() const {  // {{{
  return m_xembed.get();
}  // }}}

/**
 * Make sure that the window mapping state is correct
 */
void tray_client::ensure_state() const {  // {{{
  if (!mapped() && ((xembed()->flags & XEMBED_MAPPED) == XEMBED_MAPPED)) {
    m_connection.map_window_checked(window());
  } else if (mapped() && ((xembed()->flags & XEMBED_MAPPED) != XEMBED_MAPPED)) {
    m_connection.unmap_window_checked(window());
  }
}  // }}}

/**
 * Configure window size
 */
void tray_client::reconfigure(int16_t x, int16_t y) const {  // {{{
  uint32_t configure_mask = 0;
  uint32_t configure_values[7];
  xcb_params_configure_window_t configure_params;

  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, width, m_width);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, height, m_height);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, x, x);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, y, y);

  xutils::pack_values(configure_mask, &configure_params, configure_values);
  m_connection.configure_window_checked(window(), configure_mask, configure_values);
}  // }}}

/**
 * Respond to client resize requests
 */
void tray_client::configure_notify(int16_t x, int16_t y) const {  // {{{
  auto notify = memory_util::make_malloc_ptr<xcb_configure_notify_event_t>(32);
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

  const char* data = reinterpret_cast<const char*>(notify.get());
  m_connection.send_event_checked(false, m_window, XCB_EVENT_MASK_STRUCTURE_NOTIFY, data);
}  // }}}

// }}}
// implementation : tray_manager {{{

tray_manager::tray_manager(connection& conn, const logger& logger) : m_connection(conn), m_log(logger) {
  m_connection.attach_sink(this, 2);
  m_sinkattached = true;
}

tray_manager::~tray_manager() {
  if (m_activated)
    deactivate();
  if (m_sinkattached)
    m_connection.detach_sink(this, 2);
}

/**
 * Get the settings container
 */
const tray_settings tray_manager::settings() const {  // {{{
  return m_opts;
}  // }}}

/**
 * Initialize data
 */
void tray_manager::bootstrap(tray_settings settings) {  // {{{
  m_opts = settings;
  query_atom();
}  // }}}

/**
 * Activate systray management
 */
void tray_manager::activate() {  // {{{
  if (m_activated) {
    return;
  }

  m_log.info("Activating tray_manager");
  m_activated = true;

  try {
    create_window();
    create_bg();
    restack_window();
    set_wmhints();
    set_traycolors();
  } catch (const exception& err) {
    m_log.err(err.what());
    m_log.err("Cannot activate tray_manager... failed to setup window");
    m_activated = false;
    return;
  }

  if (!m_sinkattached) {
    m_connection.attach_sink(this, 2);
    m_sinkattached = true;
  }

  // Listen for visibility change events on the bar window
  if (!m_restacked && !g_signals::bar::visibility_change) {
    g_signals::bar::visibility_change = bind(&tray_manager::bar_visibility_change, this, std::placeholders::_1);
  }

  // Attempt to get control of the systray selection then
  // notify clients waiting for a manager.
  acquire_selection();

  // If replacing an existing manager or if re-activating from getting
  // replaced, we delay the notification broadcast to allow the clients
  // to get unembedded...
  if (m_othermanager)
    std::this_thread::sleep_for(std::chrono::seconds{1});

  notify_clients();

  m_connection.flush();
}  // }}}

/**
 * Deactivate systray management
 */
void tray_manager::deactivate() {  // {{{
  if (!m_activated) {
    return;
  }

  m_log.info("Deactivating tray_manager");
  m_activated = false;

  if (m_delayed_activation.joinable())
    m_delayed_activation.join();

  if (g_signals::tray::report_slotcount) {
    m_log.trace("tray: Report empty slotcount");
    g_signals::tray::report_slotcount(0);
  }

  if (g_signals::bar::visibility_change) {
    g_signals::bar::visibility_change = nullptr;
  }

  if (!m_connection.connection_has_error()) {
    if (m_connection.get_selection_owner_unchecked(m_atom).owner<xcb_window_t>() == m_tray) {
      m_log.trace("tray: Unset selection owner");
      m_connection.set_selection_owner(XCB_NONE, m_atom, XCB_CURRENT_TIME);
    }
  }

  m_log.trace("tray: Unembed clients");
  m_clients.clear();

  if (m_tray) {
    if (m_mapped) {
      m_log.trace("tray: Unmap window");
      m_connection.unmap_window(m_tray);
      m_mapped = false;
    }

    m_log.trace("tray: Destroy window");
    m_connection.destroy_window(m_tray);
    m_hidden = false;
  }

  if (m_pixmap) {
    m_connection.free_pixmap(m_pixmap);
  }

  if (m_gc) {
    m_connection.free_gc(m_pixmap);
  }

  m_tray = 0;
  m_pixmap = 0;
  m_gc = 0;
  m_rootpixmap.pixmap = 0;
  m_prevwidth = 0;
  m_prevheight = 0;

  m_connection.flush();
}  // }}}

/**
 * Reconfigure tray
 */
void tray_manager::reconfigure() {  // {{{
  // Skip if tray window doesn't exist or if it's
  // in pseudo-hidden state
  if (!m_tray || m_hidden) {
    return;
  }

  if (!m_mtx.try_lock()) {
    return;
  }

  std::unique_lock<std::mutex> guard(m_mtx, std::adopt_lock);

  try {
    reconfigure_clients();
  } catch (const exception& err) {
    m_log.err("Failed to reconfigure tray clients (%s)", err.what());
  }

  try {
    reconfigure_window();
  } catch (const exception& err) {
    m_log.err("Failed to reconfigure tray window (%s)", err.what());
  }

  try {
    reconfigure_bg();
  } catch (const exception& err) {
    m_log.err("Failed to reconfigure tray background (%s)", err.what());
  }

  refresh_window();

  m_connection.flush();

  m_opts.configured_slots = mapped_clients();

  // Report status
  if (g_signals::tray::report_slotcount) {
    g_signals::tray::report_slotcount(m_opts.configured_slots);
  }

  guard.unlock();

  refresh_window();
}  // }}}

/**
 * Reconfigure container window
 */
void tray_manager::reconfigure_window() {  // {{{
  m_log.trace("tray: Reconfigure window");

  if (!m_tray) {
    return;
  }

  auto clients = mapped_clients();

  if (!clients && m_mapped) {
    m_log.trace("tray: Reconfigure window / unmap");
    m_mapped = false;
    m_connection.unmap_window_checked(m_tray);
  } else if (clients && !m_mapped) {
    m_log.trace("tray: Reconfigure window / map");
    m_mapped = true;
    m_connection.map_window_checked(m_tray);
  }

  if (!m_mapped) {
    m_log.trace("tray: Reconfigure window / ignoring unmapped");
    return;
  }

  if (!clients) {
    m_log.trace("tray: Reconfigure window / no clients");
    return;
  }

  auto width = calculate_w();
  auto x = calculate_x(width);

  if (m_opts.configured_w == width && m_opts.configured_x == x) {
    m_log.trace("tray: Reconfigure window / ignoring unchanged values w=%d x=%d", width, x);
    return;
  }

  // configure window
  uint32_t mask = 0;
  uint32_t values[7];
  xcb_params_configure_window_t params;

  m_log.trace("tray: New window values, width=%d, x=%d", width, x);

  XCB_AUX_ADD_PARAM(&mask, &params, width, width);
  XCB_AUX_ADD_PARAM(&mask, &params, x, x);

  xutils::pack_values(mask, &params, values);

  m_connection.configure_window_checked(m_tray, mask, values);

  m_opts.configured_w = width;
  m_opts.configured_x = x;
}  // }}}

/**
 * Reconfigure clients
 */
void tray_manager::reconfigure_clients() {  // {{{
  m_log.trace("tray: Reconfigure clients");

  uint32_t x = m_opts.spacing;

  for (auto it = m_clients.rbegin(); it != m_clients.rend(); it++) {
    auto client = *it;

    try {
      client->ensure_state();
      client->reconfigure(x, calculate_client_y());

      x += m_opts.width + m_opts.spacing;
    } catch (const xpp::x::error::window& err) {
      remove_client(client, false);
    }
  }
}  // }}}

/**
 * Reconfigure root pixmap
 */
void tray_manager::reconfigure_bg(bool realloc) {  // {{{
  if (!m_opts.transparent || m_clients.empty() || !m_mapped) {
    return;
  }

  auto w = calculate_w();
  auto h = calculate_h();

  if ((!w || (w == m_prevwidth && h == m_prevheight)) && !realloc) {
    return;
  }

  m_log.trace("tray: Reconfigure bg (realloc=%i)", realloc);

  if (!m_rootpixmap.pixmap && !get_root_pixmap(m_connection, &m_rootpixmap)) {
    return m_log.err("Failed to get root pixmap for tray background (realloc=%i)", realloc);
  } else if (realloc && !get_root_pixmap(m_connection, &m_rootpixmap)) {
    return m_log.err("Failed to get root pixmap for tray background (realloc=%i)", realloc);
  }

  m_log.trace("tray: rootpixmap=%x (%dx%d+%d+%d), tray=%x, pixmap=%x, gc=%x", m_rootpixmap.pixmap, m_rootpixmap.width,
      m_rootpixmap.height, m_rootpixmap.x, m_rootpixmap.y, m_tray, m_pixmap, m_gc);

  m_prevwidth = w;
  m_prevheight = h;

  auto x = calculate_x(w);
  auto y = calculate_y();

  auto px = m_rootpixmap.x + x;
  auto py = m_rootpixmap.y + y;

  if (realloc) {
    vector<uint8_t> image_data;
    uint8_t image_depth;

    try {
      auto image_reply =
          m_connection.get_image(XCB_IMAGE_FORMAT_Z_PIXMAP, m_rootpixmap.pixmap, px, py, w, h, XCB_COPY_PLANE);
      image_depth = image_reply->depth;
      std::back_insert_iterator<decltype(image_data)> back_it(image_data);
      std::copy(image_reply.data().begin(), image_reply.data().end(), back_it);
    } catch (const exception& err) {
      m_log.err("Failed to get slice of root pixmap (%s)", err.what());
      return;
    }

    try {
      m_connection.put_image_checked(
          XCB_IMAGE_FORMAT_Z_PIXMAP, m_pixmap, m_gc, w, h, 0, 0, 0, image_depth, image_data.size(), image_data.data());
    } catch (const exception& err) {
      m_log.err("Failed to store slice of root pixmap (%s)", err.what());
      return;
    }
  }

  m_connection.copy_area_checked(m_rootpixmap.pixmap, m_pixmap, m_gc, px, py, 0, 0, w, h);
}  // }}}

/**
 * Refresh the bar window by clearing it along with each client window
 */
void tray_manager::refresh_window() {  // {{{
  if (!m_mtx.try_lock()) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_mtx, std::adopt_lock);

  if (!m_activated || !m_mapped || m_hidden) {
    return;
  }

  m_log.trace("tray: Refreshing window");

  auto width = calculate_w();
  auto height = calculate_h();

  if (m_opts.transparent && !m_rootpixmap.pixmap) {
    draw_util::fill(m_connection, m_pixmap, m_gc, 0, 0, width, height);
  }

  m_connection.clear_area(1, m_tray, 0, 0, width, height);

  for (auto&& client : m_clients) {
    m_connection.clear_area(1, client->window(), 0, 0, m_opts.width, height);
  }

  m_connection.flush();
}  // }}}

/**
 * Find the systray selection atom
 */
void tray_manager::query_atom() {  // {{{
  m_log.trace("tray: Find systray selection atom for the default screen");
  string name{"_NET_SYSTEM_TRAY_S" + to_string(m_connection.default_screen())};
  auto reply = m_connection.intern_atom(false, name.length(), name.c_str());
  m_atom = reply.atom();
}  // }}}

/**
 * Create tray window
 */
void tray_manager::create_window() {  // {{{
  auto scr = m_connection.screen();
  auto w = calculate_w();
  auto h = calculate_h();
  auto x = calculate_x(w);
  auto y = calculate_y();

  if (w < 1) {
    w = 1;
  }

  m_tray = m_connection.generate_id();
  m_log.trace("tray: Create tray window %s, (%ix%i+%i+%i)", m_connection.id(m_tray), w, h, x, y);

  uint32_t mask = 0;
  uint32_t values[16];
  xcb_params_cw_t params;

  if (!m_opts.transparent) {
    XCB_AUX_ADD_PARAM(&mask, &params, back_pixel, m_opts.background);
    XCB_AUX_ADD_PARAM(&mask, &params, border_pixel, m_opts.background);
  }

  XCB_AUX_ADD_PARAM(&mask, &params, backing_store, XCB_BACKING_STORE_WHEN_MAPPED);
  XCB_AUX_ADD_PARAM(&mask, &params, override_redirect, true);
  XCB_AUX_ADD_PARAM(&mask, &params, event_mask, XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY);

  xutils::pack_values(mask, &params, values);

  m_connection.create_window_checked(
      scr->root_depth, m_tray, scr->root, x, y, w, h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, scr->root_visual, mask, values);
}  // }}}

/**
 * Create tray window background components
 */
void tray_manager::create_bg(bool realloc) {  // {{{
  if (!m_opts.transparent) {
    return;
  }

  if (!realloc && m_pixmap && m_gc && m_rootpixmap.pixmap) {
    return;
  }

  if (realloc && m_pixmap) {
    m_connection.free_pixmap(m_pixmap);
    m_pixmap = 0;
  }

  if (realloc && m_gc) {
    m_connection.free_gc(m_gc);
    m_gc = 0;
  }

  auto w = m_opts.width_max;
  auto h = calculate_h();

  if (!m_pixmap && !graphics_util::create_pixmap(m_connection, m_tray, w, h, &m_pixmap)) {
    return m_log.err("Failed to create pixmap for tray background");
  } else if (!m_gc && !graphics_util::create_gc(m_connection, m_pixmap, &m_gc)) {
    return m_log.err("Failed to create gcontext for tray background");
  }

  try {
    uint32_t mask = 0;
    uint32_t values[16];
    xcb_params_cw_t params;
    XCB_AUX_ADD_PARAM(&mask, &params, back_pixmap, m_pixmap);
    xutils::pack_values(mask, &params, values);
    m_connection.change_window_attributes_checked(m_tray, mask, values);
  } catch (const exception& err) {
    m_log.err("Failed to set tray window back pixmap (%s)", err.what());
  }
}  // }}}

/**
 * Put tray window above the defined sibling in the window stack
 */
void tray_manager::restack_window() {  // {{{
  if (!m_opts.sibling) {
    return;
  }

  try {
    m_log.trace("tray: Restacking tray window");

    uint32_t mask = 0;
    uint32_t values[7];
    xcb_params_configure_window_t params;

    XCB_AUX_ADD_PARAM(&mask, &params, sibling, m_opts.sibling);
    XCB_AUX_ADD_PARAM(&mask, &params, stack_mode, XCB_STACK_MODE_ABOVE);
    xutils::pack_values(mask, &params, values);
    m_connection.configure_window_checked(m_tray, mask, values);
    m_restacked = true;
  } catch (const exception& err) {
    auto id = m_connection.id(m_opts.sibling);
    m_log.trace("tray: Failed to put tray above %s in the stack (%s)", id, err.what());
  }
}  // }}}

/**
 * Set window WM hints
 */
void tray_manager::set_wmhints() {  // {{{
  m_log.trace("tray: Set window WM_NAME / WM_CLASS", m_connection.id(m_tray));
  xcb_icccm_set_wm_name(m_connection, m_tray, XCB_ATOM_STRING, 8, 19, TRAY_WM_NAME);
  xcb_icccm_set_wm_class(m_connection, m_tray, 12, TRAY_WM_CLASS);

  m_log.trace("tray: Set window WM_PROTOCOLS");
  vector<xcb_atom_t> wm_flags;
  wm_flags.emplace_back(WM_DELETE_WINDOW);
  wm_flags.emplace_back(WM_TAKE_FOCUS);
  wm_util::set_wmprotocols(m_connection, m_tray, wm_flags);

  m_log.trace("tray: Set window _NET_WM_WINDOW_TYPE");
  vector<xcb_atom_t> types;
  types.emplace_back(_NET_WM_WINDOW_TYPE_DOCK);
  types.emplace_back(_NET_WM_WINDOW_TYPE_NORMAL);
  wm_util::set_wmprotocols(m_connection, m_tray, types);
  wm_util::set_windowtype(m_connection, m_tray, types);

  m_log.trace("tray: Set window _NET_WM_STATE");
  vector<xcb_atom_t> states;
  states.emplace_back(_NET_WM_STATE_SKIP_TASKBAR);
  m_connection.change_property(
      XCB_PROP_MODE_REPLACE, m_tray, _NET_WM_STATE, XCB_ATOM_ATOM, 32, states.size(), states.data());

  m_log.trace("tray: Set window _NET_WM_PID");
  wm_util::set_wmpid(m_connection, m_tray, getpid());

  m_log.trace("tray: Set window _NET_SYSTEM_TRAY_ORIENTATION");
  wm_util::set_trayorientation(m_connection, m_tray, _NET_SYSTEM_TRAY_ORIENTATION_HORZ);

  m_log.trace("tray: Set window _NET_SYSTEM_TRAY_VISUAL");
  wm_util::set_trayvisual(m_connection, m_tray, m_connection.screen()->root_visual);
}  // }}}

/**
 * Set color atom used by clients when determing icon theme
 */
void tray_manager::set_traycolors() {  // {{{
  m_log.trace("tray: Set _NET_SYSTEM_TRAY_COLORS to %x", m_opts.background);

  auto r = color_util::red_channel(m_opts.background);
  auto g = color_util::green_channel(m_opts.background);
  auto b = color_util::blue_channel(m_opts.background);

  const uint32_t colors[12] = {
      r, g, b,  // normal
      r, g, b,  // error
      r, g, b,  // warning
      r, g, b,  // success
  };

  m_connection.change_property(
      XCB_PROP_MODE_REPLACE, m_tray, _NET_SYSTEM_TRAY_COLORS, XCB_ATOM_CARDINAL, 32, 12, colors);
}  // }}}

/**
 * Acquire the systray selection
 */
void tray_manager::acquire_selection() {  // {{{
  xcb_window_t owner = m_connection.get_selection_owner_unchecked(m_atom)->owner;

  if (owner == m_tray) {
    m_log.info("tray: Already managing the systray selection");
    return;
  } else if ((m_othermanager = owner)) {
    m_log.info("Replacing selection manager %s", m_connection.id(owner));
  }

  m_log.trace("tray: Change selection owner to %s", m_connection.id(m_tray));
  m_connection.set_selection_owner_checked(m_tray, m_atom, XCB_CURRENT_TIME);

  if (m_connection.get_selection_owner_unchecked(m_atom)->owner != m_tray)
    throw application_error("Failed to get control of the systray selection");
}  // }}}

/**
 * Notify pending clients about the new systray MANAGER
 */
void tray_manager::notify_clients() {  // {{{
  m_log.trace("tray: Broadcast new selection manager to pending clients");
  auto message = m_connection.make_client_message(MANAGER, m_connection.root());
  message->data.data32[0] = XCB_CURRENT_TIME;
  message->data.data32[1] = m_atom;
  message->data.data32[2] = m_tray;
  m_connection.send_client_message(message, m_connection.root());
}  // }}}

/**
 * Track changes to the given selection owner
 * If it gets destroyed or goes away we can reactivate the tray_manager
 */
void tray_manager::track_selection_owner(xcb_window_t owner) {  // {{{
  if (!owner)
    return;
  m_log.trace("tray: Listen for events on the new selection window");
  const uint32_t value_list[1]{XCB_EVENT_MASK_STRUCTURE_NOTIFY};
  m_connection.change_window_attributes(owner, XCB_CW_EVENT_MASK, value_list);
}  // }}}

/**
 * Process client docking request
 */
void tray_manager::process_docking_request(xcb_window_t win) {  // {{{
  auto client = find_client(win);

  if (client) {
    m_log.warn("Tray client %s already embedded, ignoring request...", m_connection.id(win));
    return;
  }

  m_log.info("Processing docking request from %s", m_connection.id(win));

  m_clients.emplace_back(make_shared<tray_client>(m_connection, win, m_opts.width, m_opts.height));
  client = m_clients.back();

  try {
    m_log.trace("tray: Get client _XEMBED_INFO");
    xembed::query(m_connection, win, client->xembed());
  } catch (const application_error& err) {
    m_log.err(err.what());
  } catch (const xpp::x::error::window& err) {
    m_log.err("Failed to query for _XEMBED_INFO, removing client... (%s)", err.what());
    remove_client(client, false);
    return;
  }

  try {
    m_log.trace("tray: Update client window");
    {
      uint32_t mask = 0;
      uint32_t values[16];
      xcb_params_cw_t params;
      XCB_AUX_ADD_PARAM(&mask, &params, event_mask, XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY);
      XCB_AUX_ADD_PARAM(&mask, &params, back_pixmap, XCB_BACK_PIXMAP_PARENT_RELATIVE);
      xutils::pack_values(mask, &params, values);
      m_connection.change_window_attributes_checked(client->window(), mask, values);
    }

    m_log.trace("tray: Configure client size");
    client->reconfigure(0, 0);

    m_log.trace("tray: Add client window to the save set");
    m_connection.change_save_set_checked(XCB_SET_MODE_INSERT, client->window());

    m_log.trace("tray: Reparent client");
    m_connection.reparent_window_checked(
        client->window(), m_tray, calculate_client_x(client->window()), calculate_client_y());

    m_log.trace("tray: Send embbeded notification to client");
    xembed::notify_embedded(m_connection, client->window(), m_tray, client->xembed()->version);

    if (client->xembed()->flags & XEMBED_MAPPED) {
      m_log.trace("tray: Map client");
      m_connection.map_window_checked(client->window());
    }
  } catch (const xpp::x::error::window& err) {
    m_log.err("Failed to setup tray client, removing... (%s)", err.what());
    remove_client(client, false);
  }
}  // }}}

/**
 * Signal handler connected to the bar window's visibility change signal.
 * This is used as a fallback in case the window restacking fails. It will
 * toggle the tray window whenever the visibility of the bar window changes.
 */
void tray_manager::bar_visibility_change(bool state) {  // {{{
  if (m_hidden == !state) {
    return;
  }

  m_log.trace("tray: visibility_change %d", state);

  m_hidden = !state;

  if (!m_hidden && !m_mapped) {
    m_connection.map_window(m_tray);
  } else if (m_hidden && m_mapped) {
    m_connection.unmap_window(m_tray);
  } else {
    return;
  }

  m_connection.flush();
}  // }}}

/**
 * Calculate x position of tray window
 */
int16_t tray_manager::calculate_x(uint16_t width) const {  // {{{
  auto x = m_opts.orig_x;
  if (m_opts.align == alignment::RIGHT)
    x -= ((m_opts.width + m_opts.spacing) * m_clients.size() + m_opts.spacing);
  else if (m_opts.align == alignment::CENTER)
    x -= (width / 2) - (m_opts.width / 2);
  return x;
}  // }}}

/**
 * Calculate y position of tray window
 */
int16_t tray_manager::calculate_y() const {  // {{{
  return m_opts.orig_y;
}  // }}}

/**
 * Calculate width of tray window
 */
uint16_t tray_manager::calculate_w() const {  // {{{
  uint16_t width = m_opts.spacing;

  for (auto&& client : m_clients) {
    if (client->mapped()) {
      width += m_opts.spacing + m_opts.width;
    }
  }

  return width;
}  // }}}

/**
 * Calculate height of tray window
 */
uint16_t tray_manager::calculate_h() const {  // {{{
  return m_opts.height_fill;
}  // }}}

/**
 * Calculate x position of client window
 */
int16_t tray_manager::calculate_client_x(const xcb_window_t& win) {  // {{{
  for (size_t i = 0; i < m_clients.size(); i++)
    if (m_clients[i]->match(win))
      return m_opts.spacing + m_opts.width * i;
  return m_opts.spacing;
}  // }}}

/**
 * Calculate y position of client window
 */
int16_t tray_manager::calculate_client_y() {  // {{{
  return (m_opts.height_fill - m_opts.height) / 2;
}  // }}}

/**
 * Find tray client by window
 */
shared_ptr<tray_client> tray_manager::find_client(const xcb_window_t& win) const {  // {{{
  for (auto&& client : m_clients)
    if (client->match(win)) {
      return shared_ptr<tray_client>{client.get(), factory_util::null_deleter{}};
    }
  return {};
}  // }}}

/**
 * Client error handling
 */
void tray_manager::remove_client(shared_ptr<tray_client>& client, bool reconfigure) {  // {{{
  m_clients.erase(std::find(m_clients.begin(), m_clients.end(), client));

  if (reconfigure) {
    tray_manager::reconfigure();
  }
}  // }}}

/**
 * Get number of mapped clients
 */
int tray_manager::mapped_clients() const {  // {{{
  int mapped_clients = 0;

  for (auto&& client : m_clients) {
    if (client->mapped()) {
      mapped_clients++;
    }
  }

  return mapped_clients;
}  // }}}

/**
 * Event callback : XCB_EXPOSE
 */
void tray_manager::handle(const evt::expose& evt) {  // {{{
  if (m_activated && !m_clients.empty()) {
    m_log.trace("tray: Received expose event for %s", m_connection.id(evt->window));
    reconfigure_window();
  }
}  // }}}

/**
 * Event callback : XCB_VISIBILITY_NOTIFY
 */
void tray_manager::handle(const evt::visibility_notify& evt) {  // {{{
  if (m_activated && !m_clients.empty()) {
    m_log.trace("tray: Received visibility_notify for %s", m_connection.id(evt->window));
    reconfigure_window();
  }
}  // }}}

/**
 * Event callback : XCB_CLIENT_MESSAGE
 */
void tray_manager::handle(const evt::client_message& evt) {  // {{{
  if (!m_activated) {
    return;
  }

  if (evt->type == _NET_SYSTEM_TRAY_OPCODE && evt->format == 32) {
    m_log.trace("tray: Received client_message");

    switch (evt->data.data32[1]) {
      case SYSTEM_TRAY_REQUEST_DOCK:
        try {
          process_docking_request(evt->data.data32[2]);
        } catch (const exception& err) {
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
      m_tray = 0;
      deactivate();
    }
  }
}  // }}}

/**
 * Event callback : XCB_CONFIGURE_REQUEST
 *
 * Called when a tray client thinks he's part of the free world and
 * wants to reconfigure its window. This is of course nothing we appreciate
 * so we return an answer that'll put him in place.
 */
void tray_manager::handle(const evt::configure_request& evt) {  // {{{
  if (!m_activated) {
    return;
  }

  auto client = find_client(evt->window);
  if (!client) {
    return;
  }

  try {
    m_log.trace("tray: Client configure request %s", m_connection.id(evt->window));
    client->configure_notify(calculate_client_x(evt->window), calculate_client_y());
  } catch (const xpp::x::error::window& err) {
    m_log.err("Failed to reconfigure tray client, removing... (%s)", err.what());
    remove_client(client);
  }
}  // }}}

/**
 * @see tray_manager::handle(const evt::configure_request&);
 */
void tray_manager::handle(const evt::resize_request& evt) {  // {{{
  if (!m_activated) {
    return;
  }

  auto client = find_client(evt->window);
  if (!client) {
    return;
  }

  try {
    m_log.trace("tray: Received resize_request for client %s", m_connection.id(evt->window));
    client->configure_notify(calculate_client_x(evt->window), calculate_client_y());
  } catch (const xpp::x::error::window& err) {
    m_log.err("Failed to reconfigure tray client, removing... (%s)", err.what());
    remove_client(client);
  }
}  // }}}

/**
 * Event callback : XCB_SELECTION_CLEAR
 */
void tray_manager::handle(const evt::selection_clear& evt) {  // {{{
  if (!m_activated) {
    return;
  } else if (evt->selection != m_atom) {
    return;
  } else if (evt->owner != m_tray) {
    return;
  }

  try {
    m_log.warn("Lost systray selection, deactivating...");
    m_othermanager = m_connection.get_selection_owner(m_atom)->owner;
    track_selection_owner(m_othermanager);
  } catch (const exception& err) {
    m_log.err("Failed to get systray selection owner");
    m_othermanager = 0;
  }

  deactivate();
}  // }}}

/**
 * Event callback : XCB_PROPERTY_NOTIFY
 */
void tray_manager::handle(const evt::property_notify& evt) {  // {{{
  if (!m_activated) {
    return;
  } else if (evt->atom == _XROOTMAP_ID) {
    reconfigure_bg(true);
    refresh_window();
  } else if (evt->atom == _XSETROOT_ID) {
    reconfigure_bg(true);
    refresh_window();
  } else if (evt->atom == ESETROOT_PMAP_ID) {
    reconfigure_bg(true);
    refresh_window();
  } else if (evt->atom != _XEMBED_INFO) {
    auto client = find_client(evt->window);
    if (!client) {
      return;
    }

    m_log.trace("tray: _XEMBED_INFO: %s", m_connection.id(evt->window));

    auto xd = client->xembed();
    auto win = client->window();

    if (evt->state == XCB_PROPERTY_NEW_VALUE) {
      m_log.trace("tray: _XEMBED_INFO value has changed");
    }

    xembed::query(m_connection, win, xd);

    m_log.trace("tray: _XEMBED_INFO[0]=%u _XEMBED_INFO[1]=%u", xd->version, xd->flags);

    if ((client->xembed()->flags & XEMBED_MAPPED) & XEMBED_MAPPED) {
      reconfigure();
    }
  }
}  // }}}

/**
 * Event callback : XCB_REPARENT_NOTIFY
 */
void tray_manager::handle(const evt::reparent_notify& evt) {  // {{{
  if (!m_activated) {
    return;
  }

  auto client = find_client(evt->window);
  if (client && evt->parent != m_tray) {
    m_log.trace("tray: Received reparent_notify for client, remove...");
    remove_client(client);
  }
}  // }}}

/**
 * Event callback : XCB_DESTROY_NOTIFY
 */
void tray_manager::handle(const evt::destroy_notify& evt) {  // {{{
  if (!m_activated && evt->window == m_othermanager) {
    m_log.trace("tray: Received destroy_notify");
    m_log.info("Tray selection available... re-activating");
    activate();
    window{m_connection, m_tray}.redraw();
  } else if (m_activated) {
    auto client = find_client(evt->window);
    if (client) {
      m_log.trace("tray: Received destroy_notify for client, remove...");
      remove_client(client);
    }
  }
}  // }}}

/**
 * Event callback : XCB_MAP_NOTIFY
 */
void tray_manager::handle(const evt::map_notify& evt) {  // {{{
  if (!m_activated) {
    return;
  }

  if (evt->window == m_tray && !m_mapped) {
    if (m_mapped) {
      return;
    }
    m_log.trace("tray: Received map_notify");
    m_log.trace("tray: Update container mapped flag");
    m_mapped = true;
  } else {
    auto client = find_client(evt->window);
    if (client) {
      m_log.trace("tray: Received map_notify");
      m_log.trace("tray: Set client mapped");
      client->mapped(true);
    }
  }

  if (mapped_clients() > m_opts.configured_slots) {
    reconfigure();
  }
}  // }}}

/**
 * Event callback : XCB_UNMAP_NOTIFY
 */
void tray_manager::handle(const evt::unmap_notify& evt) {  // {{{
  if (!m_activated) {
    return;
  }

  if (evt->window == m_tray) {
    m_log.trace("tray: Received unmap_notify");
    if (!m_mapped) {
      return;
    }
    m_log.trace("tray: Update container mapped flag");
    m_mapped = false;
    m_opts.configured_w = 0;
    m_opts.configured_x = 0;
  } else {
    auto client = find_client(evt->window);
    if (client) {
      m_log.trace("tray: Received unmap_notify");
      m_log.trace("tray: Set client unmapped");
      client->mapped(true);
    }
  }
}  // }}}

// }}}

POLYBAR_NS_END
