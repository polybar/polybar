#include "x11/legacy_tray_manager.hpp"

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_image.h>

#include <thread>

#include "cairo/context.hpp"
#include "cairo/surface.hpp"
#include "errors.hpp"
#include "events/signal.hpp"
#include "utils/color.hpp"
#include "utils/factory.hpp"
#include "utils/math.hpp"
#include "utils/memory.hpp"
#include "utils/process.hpp"
#include "utils/units.hpp"
#include "x11/background_manager.hpp"
#include "x11/connection.hpp"
#include "x11/ewmh.hpp"
#include "x11/icccm.hpp"
#include "x11/window.hpp"
#include "x11/winspec.hpp"
#include "x11/xembed.hpp"

/**
 * Legacy tray implementation that does not run as part of a module.
 */

POLYBAR_NS

namespace legacy_tray {

/**
 * Create instance
 */
tray_manager::make_type tray_manager::make(const bar_settings& settings) {
  return std::make_unique<tray_manager>(
      connection::make(), signal_emitter::make(), logger::make(), background_manager::make(), settings);
}

tray_manager::tray_manager(connection& conn, signal_emitter& emitter, const logger& logger, background_manager& back,
    const bar_settings& settings)
    : m_connection(conn), m_sig(emitter), m_log(logger), m_background_manager(back), m_bar_opts(settings) {
  m_connection.attach_sink(this, SINK_PRIORITY_TRAY);
}

tray_manager::~tray_manager() {
  if (m_delaythread.joinable()) {
    m_delaythread.join();
  }
  m_connection.detach_sink(this, SINK_PRIORITY_TRAY);
  deactivate();
}

void tray_manager::setup(const config& conf, const string& tray_module_name) {
  auto bs = conf.section();

  for (const auto& deprecated : {"tray-position", "tray-detached", "tray-maxsize", "tray-scale", "tray-background",
           "tray-foreground", "tray-padding", "tray-offset-x", "tray-offset-y"}) {
    if (conf.has(bs, deprecated)) {
      m_log.warn("tray: %s.%s is deprecated, use the dedicated tray module to display the system tray", bs, deprecated);
    }
  }

  string position = conf.get(bs, "tray-position", "none"s);

  if (!position.empty() && position != "none" && !tray_module_name.empty()) {
    m_log.err(
        "The tray position is manually defined (`tray-position`) and also set by the tray module (%s). `tray-position` "
        "will be ignored",
        tray_module_name);
  }

  if (!tray_module_name.empty()) {
    m_opts.tray_position = tray_postition::MODULE;
    return;
  } else if (position == "left") {
    m_opts.tray_position = tray_postition::LEFT;
  } else if (position == "right") {
    m_opts.tray_position = tray_postition::RIGHT;
  } else if (position == "center") {
    m_opts.tray_position = tray_postition::CENTER;
  } else if (position != "none") {
    return m_log.err("Disabling tray manager (reason: Invalid position \"" + position + "\")");
  } else {
    return;
  }

  m_opts.detached = conf.get(bs, "tray-detached", false);
  m_opts.height = m_bar_opts.size.h;
  m_opts.height -= m_bar_opts.borders.at(edge::BOTTOM).size;
  m_opts.height -= m_bar_opts.borders.at(edge::TOP).size;
  m_opts.height_fill = m_opts.height;

  if (m_opts.height % 2 != 0) {
    m_opts.height--;
  }

  auto maxsize = conf.get<unsigned int>(bs, "tray-maxsize", 16);
  if (m_opts.height > maxsize) {
    m_opts.spacing += (m_opts.height - maxsize) / 2;
    m_opts.height = maxsize;
  }

  m_opts.width_max = m_bar_opts.size.w;
  m_opts.width = m_opts.height;
  m_opts.orig_y = m_bar_opts.pos.y + m_bar_opts.borders.at(edge::TOP).size;

  // Apply user-defined scaling
  auto scale = conf.get(bs, "tray-scale", 1.0);
  m_opts.width *= scale;
  m_opts.height_fill *= scale;

  auto inner_area = m_bar_opts.inner_area(true);

  switch (m_opts.tray_position) {
    case tray_postition::NONE:
      break;
    case tray_postition::LEFT:
      m_opts.orig_x = inner_area.x;
      break;
    case tray_postition::CENTER:
      m_opts.orig_x = inner_area.x + inner_area.width / 2 - m_opts.width / 2;
      break;
    case tray_postition::RIGHT:
      m_opts.orig_x = inner_area.x + inner_area.width;
      break;
    case tray_postition::MODULE:
      break;
  }

  if (conf.has(bs, "tray-transparent")) {
    m_log.warn("tray-transparent is deprecated, the tray always uses pseudo-transparency. Please remove it.");
  }

  // Set user-defined foreground and background colors.
  m_opts.background = conf.get(bs, "tray-background", m_bar_opts.background);
  m_opts.foreground = conf.get(bs, "tray-foreground", m_bar_opts.foreground);

  if (m_opts.background.alpha_i() != 255) {
    m_log.trace("tray: enable transparency");
    m_opts.transparent = true;
  }

  // Add user-defined padding
  m_opts.spacing += conf.get<unsigned int>(bs, "tray-padding", 0);

  // Add user-defiend offset
  auto offset_x = conf.get(bs, "tray-offset-x", percentage_with_offset{});
  auto offset_y = conf.get(bs, "tray-offset-y", percentage_with_offset{});

  int max_x;
  int max_y;

  if (m_opts.detached) {
    max_x = m_bar_opts.monitor->w;
    max_y = m_bar_opts.monitor->h;
  } else {
    max_x = inner_area.width;
    max_y = inner_area.height;
  }

  m_opts.orig_x += units_utils::percentage_with_offset_to_pixel(offset_x, max_x, m_bar_opts.dpi_x);
  m_opts.orig_y += units_utils::percentage_with_offset_to_pixel(offset_y, max_y, m_bar_opts.dpi_y);
  ;
  m_opts.rel_x = m_opts.orig_x - m_bar_opts.pos.x;
  m_opts.rel_y = m_opts.orig_y - m_bar_opts.pos.y;

  // Put the tray next to the bar in the window stack
  m_opts.sibling = m_bar_opts.x_data.window;

  // Activate the tray manager
  query_atom();
  activate();
}

/**
 * Get the settings container
 */
const tray_settings tray_manager::settings() const {
  return m_opts;
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
  m_opts.running = true;

  m_sig.attach(this);

  try {
    create_window();
    create_bg();
    restack_window();
    set_wm_hints();
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
  m_opts.running = false;

  m_sig.detach(this);

  if (!m_connection.connection_has_error() && clear_selection && m_acquired_selection) {
    m_log.trace("tray: Unset selection owner");
    m_connection.set_selection_owner(XCB_NONE, m_atom, XCB_CURRENT_TIME);
  }

  m_log.trace("tray: Unembed clients");
  m_clients.clear();

  if (m_tray) {
    m_log.trace("tray: Destroy window");
    m_connection.destroy_window(m_tray);
  }
  m_context.reset();
  m_surface.reset();
  if (m_pixmap) {
    m_connection.free_pixmap(m_pixmap);
  }
  if (m_gc) {
    m_connection.free_gc(m_pixmap);
  }

  m_tray = 0;
  m_pixmap = 0;
  m_gc = 0;
  m_prevwidth = 0;
  m_prevheight = 0;
  m_opts.configured_x = 0;
  m_opts.configured_y = 0;
  m_opts.configured_w = 0;
  m_opts.configured_h = 0;
  m_opts.configured_slots = 0;
  m_acquired_selection = false;
  m_mapped = false;

  m_connection.flush();

  m_sig.emit(signals::eventqueue::notify_forcechange{});
}

/**
 * Reconfigure tray
 */
void tray_manager::reconfigure() {
  if (!m_tray) {
    return;
  } else if (m_mtx.try_lock()) {
    std::unique_lock<mutex> guard(m_mtx, std::adopt_lock);

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

    m_opts.configured_slots = mapped_clients();
    guard.unlock();
    refresh_window();
    m_connection.flush();
  }

  m_sig.emit(signals::eventqueue::notify_forcechange{});
}

/**
 * Reconfigure container window
 */
void tray_manager::reconfigure_window() {
  m_log.trace("tray: Reconfigure window (mapped=%i, clients=%i)", static_cast<bool>(m_mapped), m_clients.size());

  if (!m_tray) {
    return;
  }

  auto clients = mapped_clients();
  if (!clients && m_mapped) {
    m_log.trace("tray: Reconfigure window / unmap");
    m_connection.unmap_window_checked(m_tray);
  } else if (clients && !m_mapped && !m_hidden) {
    m_log.trace("tray: Reconfigure window / map");
    m_connection.map_window_checked(m_tray);
  }

  auto width = calculate_w();
  auto x = calculate_x(width);

  if (m_opts.transparent) {
    xcb_rectangle_t rect{0, 0, calculate_w(), calculate_h()};
    m_bg_slice = m_background_manager.observe(rect, m_tray);
  }

  if (width > 0) {
    m_log.trace("tray: New window values, width=%d, x=%d", width, x);

    unsigned int mask = 0;
    array<uint32_t, 32> values{};
    xcb_params_configure_window_t params{};

    XCB_AUX_ADD_PARAM(&mask, &params, width, width);
    XCB_AUX_ADD_PARAM(&mask, &params, x, x);
    connection::pack_values(mask, &params, values);
    m_connection.configure_window_checked(m_tray, mask, values.data());
  }

  m_opts.configured_w = width;
  m_opts.configured_x = x;
}

/**
 * Reconfigure clients
 */
void tray_manager::reconfigure_clients() {
  m_log.trace("tray: Reconfigure clients");

  int x = m_opts.spacing;

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
}

/**
 * Reconfigure root pixmap
 */
void tray_manager::reconfigure_bg(bool realloc) {
  if (!m_opts.transparent || m_clients.empty() || !m_mapped) {
    return;
  };

  m_log.trace("tray: Reconfigure bg (realloc=%i)", realloc);

  if (!m_context) {
    return m_log.err("tray: no context for drawing the background");
  }

  cairo::surface* surface = m_bg_slice->get_surface();
  if (!surface) {
    return;
  }

  m_context->clear();
  *m_context << CAIRO_OPERATOR_SOURCE << *m_surface;
  cairo_set_source_surface(*m_context, *surface, 0, 0);
  m_context->paint();
  *m_context << CAIRO_OPERATOR_OVER << m_opts.background;
  m_context->paint();
}

/**
 * Refresh the bar window by clearing it along with each client window
 */
void tray_manager::refresh_window() {
  if (!m_activated || !m_mapped || !m_mtx.try_lock()) {
    return;
  }

  std::lock_guard<mutex> lock(m_mtx, std::adopt_lock);

  m_log.trace("tray: Refreshing window");

  auto width = calculate_w();
  auto height = calculate_h();

  if (m_opts.transparent && !m_context) {
    xcb_rectangle_t rect{0, 0, static_cast<uint16_t>(width), static_cast<uint16_t>(height)};
    m_connection.poly_fill_rectangle(m_pixmap, m_gc, 1, &rect);
  }

  if (m_surface) {
    m_surface->flush();
  }

  m_connection.clear_area(0, m_tray, 0, 0, width, height);

  for (auto&& client : m_clients) {
    try {
      if (client->mapped()) {
        client->clear_window();
      }
    } catch (const std::exception& e) {
      m_log.err("Failed to clear tray client %s '%s' (%s)", m_connection.id(client->window()),
          ewmh_util::get_wm_name(client->window()), e.what());
    }
  }

  m_connection.flush();

  if (!mapped_clients()) {
    m_opts.configured_w = 0;
  } else {
    m_opts.configured_w = width;
  }
}

/**
 * Redraw window
 */
void tray_manager::redraw_window(bool realloc_bg) {
  m_log.info("Redraw tray container (id=%s)", m_connection.id(m_tray));
  reconfigure_bg(realloc_bg);
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
 * Create tray window
 */
void tray_manager::create_window() {
  m_log.trace("tray: Create tray window");

  // clang-format off
  auto win = winspec(m_connection, m_tray)
    << cw_size(calculate_w(), calculate_h())
    << cw_pos(calculate_x(calculate_w()), calculate_y())
    << cw_class(XCB_WINDOW_CLASS_INPUT_OUTPUT)
    << cw_params_backing_store(XCB_BACKING_STORE_WHEN_MAPPED)
    << cw_params_event_mask(XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
        |XCB_EVENT_MASK_STRUCTURE_NOTIFY
        |XCB_EVENT_MASK_EXPOSURE)
    << cw_params_override_redirect(true);
  // clang-format on

  if (!m_opts.transparent) {
    win << cw_params_back_pixel(m_opts.background);
    win << cw_params_border_pixel(m_opts.background);
  }

  m_tray = win << cw_flush(true);
  m_log.info("Tray window: %s", m_connection.id(m_tray));

  // activate the background manager if we have transparency
  if (m_opts.transparent) {
    xcb_rectangle_t rect{0, 0, calculate_w(), calculate_h()};
    m_bg_slice = m_background_manager.observe(rect, m_tray);
  }

  const unsigned int shadow{0};
  m_connection.change_property(XCB_PROP_MODE_REPLACE, m_tray, _COMPTON_SHADOW, XCB_ATOM_CARDINAL, 32, 1, &shadow);
}

/**
 * Create tray window background components
 */
void tray_manager::create_bg(bool realloc) {
  if (!m_opts.transparent) {
    return;
  }
  if (!realloc && m_pixmap && m_gc && m_surface && m_context) {
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

  if (realloc && m_surface) {
    m_surface.reset();
  }
  if (realloc && m_context) {
    m_context.reset();
  }

  auto w = m_opts.width_max;
  auto h = calculate_h();

  if (!m_pixmap) {
    try {
      m_pixmap = m_connection.generate_id();
      m_connection.create_pixmap_checked(m_connection.screen()->root_depth, m_pixmap, m_tray, w, h);
    } catch (const exception& err) {
      return m_log.err("Failed to create pixmap for tray background (err: %s)", err.what());
    }
  }

  if (!m_gc) {
    try {
      xcb_params_gc_t params{};
      unsigned int mask = 0;
      array<uint32_t, 32> values{};
      XCB_AUX_ADD_PARAM(&mask, &params, graphics_exposures, 1);
      connection::pack_values(mask, &params, values);
      m_gc = m_connection.generate_id();
      m_connection.create_gc_checked(m_gc, m_pixmap, mask, values.data());
    } catch (const exception& err) {
      return m_log.err("Failed to create gcontext for tray background (err: %s)", err.what());
    }
  }

  if (!m_surface) {
    xcb_visualtype_t* visual = m_connection.visual_type_for_id(m_connection.screen()->root_visual);
    if (!visual) {
      return m_log.err("Failed to get root visual for tray background");
    }
    m_surface = make_unique<cairo::xcb_surface>(m_connection, m_pixmap, visual, w, h);
  }

  if (!m_context) {
    m_context = make_unique<cairo::context>(*m_surface, m_log);
    m_context->clear();
    *m_context << CAIRO_OPERATOR_SOURCE << m_opts.background;
    m_context->paint();
  }

  try {
    m_connection.change_window_attributes_checked(m_tray, XCB_CW_BACK_PIXMAP, &m_pixmap);
  } catch (const exception& err) {
    m_log.err("Failed to set tray window back pixmap (%s)", err.what());
  }
}

/**
 * Put tray window above the defined sibling in the window stack
 */
void tray_manager::restack_window() {
  if (m_opts.sibling == XCB_NONE) {
    return;
  }

  try {
    m_log.trace("tray: Restacking tray window");

    unsigned int mask = 0;
    array<uint32_t, 32> values{};
    xcb_params_configure_window_t params{};

    XCB_AUX_ADD_PARAM(&mask, &params, sibling, m_opts.sibling);
    XCB_AUX_ADD_PARAM(&mask, &params, stack_mode, XCB_STACK_MODE_ABOVE);

    connection::pack_values(mask, &params, values);
    m_connection.configure_window_checked(m_tray, mask, values.data());
  } catch (const exception& err) {
    auto id = m_connection.id(m_opts.sibling);
    m_log.err("tray: Failed to put tray above %s in the stack (%s)", id, err.what());
  }
}

/**
 * Set window WM hints
 */
void tray_manager::set_wm_hints() {
  const unsigned int visual{m_connection.screen()->root_visual};
  const unsigned int orientation{_NET_SYSTEM_TRAY_ORIENTATION_HORZ};

  m_log.trace("bar: Set window WM_NAME / WM_CLASS");
  icccm_util::set_wm_name(m_connection, m_tray, TRAY_WM_NAME, 19_z, TRAY_WM_CLASS, 12_z);

  m_log.trace("tray: Set window WM_PROTOCOLS");
  icccm_util::set_wm_protocols(m_connection, m_tray, {WM_DELETE_WINDOW, WM_TAKE_FOCUS});

  m_log.trace("tray: Set window _NET_WM_WINDOW_TYPE");
  ewmh_util::set_wm_window_type(m_tray, {_NET_WM_WINDOW_TYPE_DOCK, _NET_WM_WINDOW_TYPE_NORMAL});

  m_log.trace("tray: Set window _NET_WM_STATE");
  ewmh_util::set_wm_state(m_tray, {_NET_WM_STATE_SKIP_TASKBAR});

  m_log.trace("tray: Set window _NET_WM_PID");
  ewmh_util::set_wm_pid(m_tray);

  m_log.trace("tray: Set window _NET_SYSTEM_TRAY_VISUAL");
  xcb_change_property(
      m_connection, XCB_PROP_MODE_REPLACE, m_tray, _NET_SYSTEM_TRAY_VISUAL, XCB_ATOM_VISUALID, 32, 1, &visual);

  m_log.trace("tray: Set window _NET_SYSTEM_TRAY_ORIENTATION");
  xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_tray, _NET_SYSTEM_TRAY_ORIENTATION,
      _NET_SYSTEM_TRAY_ORIENTATION, 32, 1, &orientation);
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
      XCB_PROP_MODE_REPLACE, m_tray, _NET_SYSTEM_TRAY_COLORS, XCB_ATOM_CARDINAL, 32, 12, colors);
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

  if (owner == m_tray) {
    m_log.trace("tray: Already managing the systray selection");
    m_acquired_selection = true;
  } else if ((m_othermanager = owner) != XCB_NONE) {
    m_log.warn("Systray selection already managed (window=%s)", m_connection.id(owner));
    track_selection_owner(m_othermanager);
  } else {
    m_log.trace("tray: Change selection owner to %s", m_connection.id(m_tray));
    m_connection.set_selection_owner_checked(m_tray, m_atom, XCB_CURRENT_TIME);
    if (m_connection.get_selection_owner_unchecked(m_atom)->owner != m_tray) {
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
    message.data.data32[2] = m_tray;
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
    const unsigned int mask{XCB_CW_EVENT_MASK};
    const unsigned int values[]{XCB_EVENT_MASK_STRUCTURE_NOTIFY};
    m_connection.change_window_attributes(owner, mask, values);
  }
}

/**
 * Process client docking request
 */
void tray_manager::process_docking_request(xcb_window_t win) {
  m_log.info("Processing docking request from '%s' (%s)", ewmh_util::get_wm_name(win), m_connection.id(win));

  m_clients.emplace_back(std::make_shared<tray_client>(m_connection, win, m_opts.width, m_opts.height));
  auto& client = m_clients.back();

  try {
    client->query_xembed();
  } catch (const xpp::x::error::window& err) {
    m_log.err("Failed to query _XEMBED_INFO, removing client... (%s)", err.what());
    remove_client(win, true);
    return;
  }

  m_log.trace("tray: xembed = %s", client->is_xembed_supported() ? "true" : "false");
  if (client->is_xembed_supported()) {
    m_log.trace("tray: version = 0x%x, flags = 0x%x, XEMBED_MAPPED = %s", client->get_xembed().get_version(),
        client->get_xembed().get_flags(), client->get_xembed().is_mapped() ? "true" : "false");
  }

  try {
    const unsigned int mask = XCB_CW_EVENT_MASK;
    const unsigned int values[]{XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY};

    m_log.trace("tray: Update client window");
    m_connection.change_window_attributes_checked(client->window(), mask, values);

    m_log.trace("tray: Configure client size");
    client->reconfigure(0, 0);

    m_log.trace("tray: Add client window to the save set");
    m_connection.change_save_set_checked(XCB_SET_MODE_INSERT, client->window());

    m_log.trace("tray: Reparent client");
    m_connection.reparent_window_checked(
        client->window(), m_tray, calculate_client_x(client->window()), calculate_client_y());

    if (client->is_xembed_supported()) {
      m_log.trace("tray: Send embbeded notification to client");
      xembed::notify_embedded(m_connection, client->window(), m_tray, client->get_xembed().get_version());
    }

    if (!client->is_xembed_supported() || client->get_xembed().is_mapped()) {
      m_log.trace("tray: Map client");
      m_connection.map_window_checked(client->window());
    }

  } catch (const std::exception& err) {
    m_log.err("Failed to setup tray client removing... (%s)", err.what());
    remove_client(win, false);
  }
}

/**
 * Calculate x position of tray window
 */
int tray_manager::calculate_x(unsigned int width) const {
  auto x = m_opts.orig_x;
  if (m_opts.tray_position == tray_postition::RIGHT) {
    x -= ((m_opts.width + m_opts.spacing) * m_clients.size() + m_opts.spacing);
  } else if (m_opts.tray_position == tray_postition::CENTER) {
    x -= (width / 2) - (m_opts.width / 2);
  }
  return x;
}

/**
 * Calculate y position of tray window
 */
int tray_manager::calculate_y(bool abspos) const {
  return abspos ? m_opts.orig_y : m_opts.rel_y;
}

/**
 * Calculate width of tray window
 */
unsigned short int tray_manager::calculate_w() const {
  unsigned int width = m_opts.spacing;
  unsigned int count{0};
  for (auto&& client : m_clients) {
    if (client->mapped()) {
      count++;
      width += m_opts.spacing + m_opts.width;
    }
  }
  return count ? width : 0;
}

/**
 * Calculate height of tray window
 */
unsigned short int tray_manager::calculate_h() const {
  return m_opts.height_fill;
}

/**
 * Calculate x position of client window
 */
int tray_manager::calculate_client_x(const xcb_window_t& win) {
  for (unsigned int i = 0; i < m_clients.size(); i++) {
    if (m_clients[i]->match(win)) {
      return m_opts.spacing + m_opts.width * i;
    }
  }
  return m_opts.spacing;
}

/**
 * Calculate y position of client window
 */
int tray_manager::calculate_client_y() {
  return (m_opts.height_fill - m_opts.height) / 2;
}

/**
 * Check if the given window is embedded
 */
bool tray_manager::is_embedded(const xcb_window_t& win) const {
  return m_clients.end() != std::find_if(m_clients.begin(), m_clients.end(),
                                [win](shared_ptr<tray_client> client) { return client->match(win); });
}

/**
 * Find tray client by window
 */
shared_ptr<tray_client> tray_manager::find_client(const xcb_window_t& win) const {
  for (auto&& client : m_clients) {
    if (client->match(win)) {
      return client;
    }
  }
  return nullptr;
}

/**
 * Remove tray client
 */
void tray_manager::remove_client(shared_ptr<tray_client>& client, bool reconfigure) {
  remove_client(client->window(), reconfigure);
}

/**
 * Remove tray client by window
 */
void tray_manager::remove_client(xcb_window_t win, bool reconfigure) {
  m_clients.erase(std::remove_if(
      m_clients.begin(), m_clients.end(), [win](shared_ptr<tray_client> client) { return client->match(win); }));

  if (reconfigure) {
    tray_manager::reconfigure();
  }
}

/**
 * Get number of mapped clients
 */
unsigned int tray_manager::mapped_clients() const {
  unsigned int mapped_clients = 0;

  for (auto&& client : m_clients) {
    if (client->mapped()) {
      mapped_clients++;
    }
  }

  return mapped_clients;
}

bool tray_manager::change_visibility(bool visible) {
  unsigned int clients{mapped_clients()};

  m_log.trace("tray: visibility_change (state=%i, activated=%i, mapped=%i, hidden=%i)", visible,
      static_cast<bool>(m_activated), static_cast<bool>(m_mapped), static_cast<bool>(m_hidden));

  m_hidden = !visible;

  if (!m_activated) {
    return false;
  } else if (!m_hidden && !m_mapped && clients) {
    m_connection.map_window(m_tray);
  } else if ((!clients || m_hidden) && m_mapped) {
    m_connection.unmap_window(m_tray);
  } else if (m_mapped && !m_hidden && clients) {
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
  } else if (evt->type == WM_PROTOCOLS && evt->data.data32[0] == WM_DELETE_WINDOW && evt->window == m_tray) {
    m_log.notice("Received WM_DELETE");
    m_tray = 0;
    deactivate();
  } else if (evt->type == _NET_SYSTEM_TRAY_OPCODE && evt->format == 32) {
    m_log.trace("tray: Received client_message");

    if (SYSTEM_TRAY_REQUEST_DOCK == evt->data.data32[1]) {
      if (!is_embedded(evt->data.data32[2])) {
        process_docking_request(evt->data.data32[2]);
      } else {
        auto win = evt->data.data32[2];
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
      find_client(evt->window)->configure_notify(calculate_client_x(evt->window), calculate_client_y());
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
      find_client(evt->window)->configure_notify(calculate_client_x(evt->window), calculate_client_y());
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
  } else if (evt->owner != m_tray) {
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

  // React an wallpaper change, if bar has transparency
  if (m_opts.transparent &&
      (evt->atom == _XROOTPMAP_ID || evt->atom == _XSETROOT_ID || evt->atom == ESETROOT_PMAP_ID)) {
    redraw_window(true);
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

  auto win = client->window();

  if (evt->state == XCB_PROPERTY_NEW_VALUE) {
    m_log.trace("tray: _XEMBED_INFO value has changed");
  }

  try {
    client->query_xembed();
  } catch (const xpp::x::error::window& err) {
    m_log.err("Failed to query _XEMBED_INFO, removing client... (%s)", err.what());
    remove_client(win, true);
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
  if (m_activated && is_embedded(evt->window) && evt->parent != m_tray) {
    m_log.trace("tray: Received reparent_notify for client, remove...");
    remove_client(evt->window);
  }
}

/**
 * Event callback : XCB_DESTROY_NOTIFY
 */
void tray_manager::handle(const evt::destroy_notify& evt) {
  if (m_activated && evt->window == m_tray) {
    deactivate();
  } else if (!m_activated && evt->window == m_othermanager) {
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
  if (m_activated && evt->window == m_tray) {
    m_log.trace("tray: Received map_notify");
    m_log.trace("tray: Update container mapped flag");
    m_mapped = true;
    redraw_window();
  } else if (is_embedded(evt->window)) {
    m_log.trace("tray: Received map_notify");
    m_log.trace("tray: Set client mapped");
    find_client(evt->window)->mapped(true);
    unsigned int clientcount{mapped_clients()};
    if (clientcount > m_opts.configured_slots) {
      reconfigure();
    }
  }
}

/**
 * Event callback : XCB_UNMAP_NOTIFY
 */
void tray_manager::handle(const evt::unmap_notify& evt) {
  if (m_activated && evt->window == m_tray) {
    m_log.trace("tray: Received unmap_notify");
    m_log.trace("tray: Update container mapped flag");
    m_mapped = false;
  } else if (m_activated && is_embedded(evt->window)) {
    m_log.trace("tray: Received unmap_notify");
    m_log.trace("tray: Set client unmapped");
    find_client(evt->window)->mapped(false);
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

bool tray_manager::on(const signals::ui::dim_window& evt) {
  if (m_activated) {
    ewmh_util::set_wm_window_opacity(m_tray, evt.cast() * 0xFFFFFFFF);
  }
  // let the event bubble
  return false;
}

bool tray_manager::on(const signals::ui::update_background&) {
  redraw_window(true);

  return false;
}

bool tray_manager::on(const signals::ui_tray::tray_pos_change& evt) {
  m_opts.orig_x =
      m_bar_opts.inner_area(true).x + std::max(0, std::min(evt.cast(), (int)(m_bar_opts.size.w - calculate_w())));

  reconfigure_window();

  return true;
}

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
  array<uint32_t, 32> configure_values{};
  xcb_params_configure_window_t configure_params{};

  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, width, m_width);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, height, m_height);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, x, x);
  XCB_AUX_ADD_PARAM(&configure_mask, &configure_params, y, y);

  connection::pack_values(configure_mask, &configure_params, configure_values);
  m_connection.configure_window_checked(window(), configure_mask, configure_values.data());
}

/**
 * Respond to client resize requests
 */
void tray_client::configure_notify(int x, int y) const {
  xcb_configure_notify_event_t notify;
  notify.response_type = XCB_CONFIGURE_NOTIFY;
  notify.event = m_window;
  notify.window = m_window;
  notify.override_redirect = false;
  notify.above_sibling = 0;
  notify.x = x;
  notify.y = y;
  notify.width = m_width;
  notify.height = m_height;
  notify.border_width = 0;

  unsigned int mask{XCB_EVENT_MASK_STRUCTURE_NOTIFY};
  m_connection.send_event_checked(false, m_window, mask, reinterpret_cast<const char*>(&notify));
}

} // namespace legacy_tray

POLYBAR_NS_END
