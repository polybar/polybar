#include <xcb/xcb_icccm.h>
#include <algorithm>
#include <string>

#include "components/bar.hpp"
#include "components/config.hpp"
#include "components/parser.hpp"
#include "components/renderer.hpp"
#include "components/screen.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "utils/bspwm.hpp"
#include "utils/color.hpp"
#include "utils/factory.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/fonts.hpp"
#include "x11/tray_manager.hpp"
#include "x11/wm.hpp"
#include "x11/xutils.hpp"

#if ENABLE_I3
#include "utils/i3.hpp"
#endif

POLYBAR_NS

using namespace signals::ui;
using namespace wm_util;

/**
 * Create instance
 */
bar::make_type bar::make() {
  // clang-format off
  return factory_util::unique<bar>(
        connection::make(),
        signal_emitter::make(),
        config::make(),
        logger::make(),
        screen::make(),
        tray_manager::make());
  // clang-format on
}

/**
 * Construct bar instance
 *
 * TODO: Break out all tray handling
 */
bar::bar(connection& conn, signal_emitter& emitter, const config& config, const logger& logger,
    unique_ptr<screen> screen, unique_ptr<tray_manager> tray_manager)
    : m_connection(conn)
    , m_sig(emitter)
    , m_conf(config)
    , m_log(logger)
    , m_screen(move(screen))
    , m_tray(move(tray_manager)) {
  string bs{m_conf.section()};

  // Get available RandR outputs
  auto monitor_name = m_conf.get<string>(bs, "monitor", "");
  auto monitor_strictmode = m_conf.get<bool>(bs, "monitor-strict", false);
  auto monitors = randr_util::get_monitors(m_connection, m_connection.screen()->root, monitor_strictmode);

  if (monitors.empty()) {
    throw application_error("No monitors found");
  } else if (monitor_name.empty()) {
    monitor_name = monitors[0]->name;
    m_log.warn("No monitor specified, using \"%s\"", monitor_name);
  }

  // Match against the defined monitor name
  for (auto&& monitor : monitors) {
    if (monitor->match(monitor_name, monitor_strictmode)) {
      m_opts.monitor = move(monitor);
      break;
    }
  }

  if (!m_opts.monitor) {
    throw application_error("Monitor \"" + monitor_name + "\" not found or disconnected");
  }

  m_log.trace("bar: Loaded monitor %s (%ix%i+%i+%i)", m_opts.monitor->name, m_opts.monitor->w, m_opts.monitor->h,
      m_opts.monitor->x, m_opts.monitor->y);

  try {
    m_opts.override_redirect = m_conf.get<bool>(bs, "dock");
    m_conf.warn_deprecated(bs, "dock", "override-redirect");
  } catch (const key_error& err) {
    m_opts.override_redirect = m_conf.get<bool>(bs, "override-redirect", m_opts.override_redirect);
  }

  // Build WM_NAME
  m_opts.wmname = m_conf.get<string>(bs, "wm-name", "polybar-" + bs.substr(4) + "_" + m_opts.monitor->name);
  m_opts.wmname = string_util::replace(m_opts.wmname, " ", "-");

  // Load configuration values
  m_opts.origin = m_conf.get<bool>(bs, "bottom", false) ? edge::BOTTOM : edge::TOP;
  m_opts.spacing = m_conf.get<decltype(m_opts.spacing)>(bs, "spacing", m_opts.spacing);
  m_opts.padding.left = m_conf.get<decltype(m_opts.padding.left)>(bs, "padding-left", m_opts.padding.left);
  m_opts.padding.right = m_conf.get<decltype(m_opts.padding.right)>(bs, "padding-right", m_opts.padding.right);
  m_opts.module_margin.left =
      m_conf.get<decltype(m_opts.module_margin.left)>(bs, "module-margin-left", m_opts.module_margin.left);
  m_opts.module_margin.right =
      m_conf.get<decltype(m_opts.module_margin.right)>(bs, "module-margin-right", m_opts.module_margin.right);
  m_opts.separator = string_util::trim(m_conf.get<string>(bs, "separator", ""), '"');
  m_opts.locale = m_conf.get<string>(bs, "locale", "");

  // Load values used to adjust the struts atom
  m_opts.strut.top = m_conf.get<int>("global/wm", "margin-top", 0);
  m_opts.strut.bottom = m_conf.get<int>("global/wm", "margin-bottom", 0);

  // Load commands used for fallback click handlers
  vector<action> actions;
  actions.emplace_back(action{mousebtn::LEFT, m_conf.get<string>(bs, "click-left", "")});
  actions.emplace_back(action{mousebtn::MIDDLE, m_conf.get<string>(bs, "click-middle", "")});
  actions.emplace_back(action{mousebtn::RIGHT, m_conf.get<string>(bs, "click-right", "")});
  actions.emplace_back(action{mousebtn::SCROLL_UP, m_conf.get<string>(bs, "scroll-up", "")});
  actions.emplace_back(action{mousebtn::SCROLL_DOWN, m_conf.get<string>(bs, "scroll-down", "")});

  for (auto&& act : actions) {
    if (!act.command.empty()) {
      m_opts.actions.emplace_back(action{act.button, act.command});
    }
  }

  // Load foreground/background
  m_opts.background = color::parse(m_conf.get<string>(bs, "background", color_util::hex<uint16_t>(m_opts.background)));
  m_opts.foreground = color::parse(m_conf.get<string>(bs, "foreground", color_util::hex<uint16_t>(m_opts.foreground)));

  // Load over-/underline color and size (warn about deprecated params if used)
  m_conf.warn_deprecated(bs, "linecolor", "{underline,overline}-color");
  m_conf.warn_deprecated(bs, "lineheight", "{underline,overline}-size");

  auto linecolor = color::parse(m_conf.get<string>(bs, "linecolor", "#f00"));
  auto lineheight = m_conf.get<int>(bs, "lineheight", 0);
  m_opts.overline.size = m_conf.get<int16_t>(bs, "overline-size", lineheight);
  m_opts.overline.color = color::parse(m_conf.get<string>(bs, "overline-color", linecolor));
  m_opts.underline.size = m_conf.get<uint16_t>(bs, "underline-size", lineheight);
  m_opts.underline.color = color::parse(m_conf.get<string>(bs, "underline-color", linecolor));

  // Load border settings
  auto bsize = m_conf.get<int>(bs, "border-size", 0);
  auto bcolor = m_conf.get<string>(bs, "border-color", "#00000000");

  m_opts.borders.emplace(edge::TOP, border_settings{});
  m_opts.borders[edge::TOP].size = m_conf.get<int>(bs, "border-top", bsize);
  m_opts.borders[edge::TOP].color = color::parse(m_conf.get<string>(bs, "border-top-color", bcolor));
  m_opts.borders.emplace(edge::BOTTOM, border_settings{});
  m_opts.borders[edge::BOTTOM].size = m_conf.get<int>(bs, "border-bottom", bsize);
  m_opts.borders[edge::BOTTOM].color = color::parse(m_conf.get<string>(bs, "border-bottom-color", bcolor));
  m_opts.borders.emplace(edge::LEFT, border_settings{});
  m_opts.borders[edge::LEFT].size = m_conf.get<int>(bs, "border-left", bsize);
  m_opts.borders[edge::LEFT].color = color::parse(m_conf.get<string>(bs, "border-left-color", bcolor));
  m_opts.borders.emplace(edge::RIGHT, border_settings{});
  m_opts.borders[edge::RIGHT].size = m_conf.get<int>(bs, "border-right", bsize);
  m_opts.borders[edge::RIGHT].color = color::parse(m_conf.get<string>(bs, "border-right-color", bcolor));

  // Load geometry values
  auto w = m_conf.get<string>(m_conf.section(), "width", "100%");
  auto h = m_conf.get<string>(m_conf.section(), "height", "24");
  auto offsetx = m_conf.get<string>(m_conf.section(), "offset-x", "");
  auto offsety = m_conf.get<string>(m_conf.section(), "offset-y", "");

  if ((m_opts.size.w = atoi(w.c_str())) && w.find('%') != string::npos) {
    m_opts.size.w = math_util::percentage_to_value<int>(m_opts.size.w, m_opts.monitor->w);
  }
  if ((m_opts.size.h = atoi(h.c_str())) && h.find('%') != string::npos) {
    m_opts.size.h = math_util::percentage_to_value<int>(m_opts.size.h, m_opts.monitor->h);
  }
  if ((m_opts.offset.x = atoi(offsetx.c_str())) != 0 && offsetx.find('%') != string::npos) {
    m_opts.offset.x = math_util::percentage_to_value<int>(m_opts.offset.x, m_opts.monitor->w);
  }
  if ((m_opts.offset.y = atoi(offsety.c_str())) != 0 && offsety.find('%') != string::npos) {
    m_opts.offset.y = math_util::percentage_to_value<int>(m_opts.offset.y, m_opts.monitor->h);
  }

  // Apply offsets
  m_opts.pos.x = m_opts.offset.x + m_opts.monitor->x;
  m_opts.pos.y = m_opts.offset.y + m_opts.monitor->y;
  m_opts.size.h += m_opts.borders[edge::TOP].size;
  m_opts.size.h += m_opts.borders[edge::BOTTOM].size;

  if (m_opts.origin == edge::BOTTOM) {
    m_opts.pos.y = m_opts.monitor->y + m_opts.monitor->h - m_opts.size.h - m_opts.offset.y;
  }

  if (m_opts.size.w <= 0 || m_opts.size.w > m_opts.monitor->w) {
    throw application_error("Resulting bar width is out of bounds");
  } else if (m_opts.size.h <= 0 || m_opts.size.h > m_opts.monitor->h) {
    throw application_error("Resulting bar height is out of bounds");
  }

  m_opts.size.w = math_util::cap<int>(m_opts.size.w, 0, m_opts.monitor->w);
  m_opts.size.h = math_util::cap<int>(m_opts.size.h, 0, m_opts.monitor->h);

  m_opts.center.y = m_opts.size.h;
  m_opts.center.y -= m_opts.borders[edge::BOTTOM].size;
  m_opts.center.y /= 2;
  m_opts.center.y += m_opts.borders[edge::TOP].size;

  m_opts.center.x = m_opts.size.w;
  m_opts.center.x -= m_opts.borders[edge::RIGHT].size;
  m_opts.center.x /= 2;
  m_opts.center.x += m_opts.borders[edge::LEFT].size;

  m_log.trace("bar: Create renderer");
  auto fonts = m_conf.get_list<string>(m_conf.section(), "font", {});
  m_renderer = renderer::make(m_opts, move(fonts));

  m_log.trace("bar: Attaching sink to registry");
  m_connection.attach_sink(this, SINK_PRIORITY_BAR);

  m_log.info("Bar geometry: %ix%i+%i+%i", m_opts.size.w, m_opts.size.h, m_opts.pos.x, m_opts.pos.y);
  m_opts.window = m_renderer->window();

  m_log.info("Bar window: %s", m_connection.id(m_opts.window));
  restack_window();

  m_log.trace("bar: Reconfigure window");
  reconfigure_struts();
  reconfigure_wm_hints();

  m_log.trace("bar: Map window");
  m_connection.map_window_checked(m_opts.window);

  // Reconfigure window position after mapping (required by Openbox)
  // Required by Openbox
  reconfigure_pos();

  m_log.trace("bar: Drawing empty bar");
  m_renderer->begin();
  m_renderer->fill_background();
  m_renderer->end();

  m_log.trace("bar: Setup tray manager");
  m_tray->setup(static_cast<const bar_settings&>(m_opts));

  broadcast_visibility();
}

/**
 * Cleanup signal handlers and destroy the bar window
 */
bar::~bar() {
  std::lock_guard<std::mutex> guard(m_mutex);
  m_connection.detach_sink(this, SINK_PRIORITY_BAR);
}

/**
 * Get the bar settings container
 */
const bar_settings bar::settings() const {
  return m_opts;
}

/**
 * Parse input string and redraw the bar window
 *
 * @param data Input string
 * @param force Unless true, do not parse unchanged data
 */
void bar::parse(const string& data, bool force) {
  if (!m_mutex.try_lock()) {
    return;
  }

  std::lock_guard<std::mutex> guard(m_mutex, std::adopt_lock);

  if (data == m_lastinput && !force) {
    return;
  }

  m_lastinput = data;

  m_log.info("Redrawing bar window");
  m_renderer->begin();

  if (m_tray && !m_tray->settings().detached && m_tray->settings().configured_slots) {
    if (m_tray->settings().align == alignment::LEFT) {
      m_renderer->reserve_space(edge::LEFT, m_tray->settings().configured_w);
    } else if (m_tray->settings().align == alignment::RIGHT) {
      m_renderer->reserve_space(edge::RIGHT, m_tray->settings().configured_w);
    }
  }

  m_renderer->fill_background();

  try {
    if (!data.empty()) {
      parser parser{m_sig, m_log, m_opts};
      parser(data);
    }
  } catch (const parser_error& err) {
    m_log.err("Failed to parse contents (reason: %s)", err.what());
  }

  m_renderer->end();
}

/**
 * Move the bar window above defined sibling
 * in the X window stack
 */
void bar::restack_window() {
  string wm_restack;

  try {
    wm_restack = m_conf.get<string>(m_conf.section(), "wm-restack");
  } catch (const key_error& err) {
    return;
  }

  auto restacked = false;

  if (wm_restack == "bspwm") {
    restacked = bspwm_util::restack_above_root(m_connection, m_opts.monitor, m_opts.window);
#if ENABLE_I3
  } else if (wm_restack == "i3" && m_opts.override_redirect) {
    restacked = i3_util::restack_above_root(m_connection, m_opts.monitor, m_opts.window);
  } else if (wm_restack == "i3" && !m_opts.override_redirect) {
    m_log.warn("Ignoring restack of i3 window (not needed when `override-redirect = false`)");
    wm_restack.clear();
#endif
  } else {
    m_log.warn("Ignoring unsupported wm-restack option '%s'", wm_restack);
    wm_restack.clear();
  }

  if (restacked) {
    m_log.info("Successfully restacked bar window");
  } else if (!wm_restack.empty()) {
    m_log.err("Failed to restack bar window");
  }
}

/**
 * Reconfigure window position
 */
void bar::reconfigure_pos() {
  window win{m_connection, m_opts.window};
  win.reconfigure_pos(m_opts.pos.x, m_opts.pos.y);
}

/**
 * Reconfigure window strut values
 */
void bar::reconfigure_struts() {
  auto geom = m_connection.get_geometry(m_screen->root());
  auto w = m_opts.size.w + m_opts.offset.x;
  auto h = m_opts.size.h + m_opts.offset.y;

  if (m_opts.origin == edge::BOTTOM) {
    h += m_opts.strut.top;
  } else {
    h += m_opts.strut.bottom;
  }

  if (m_opts.origin == edge::BOTTOM && m_opts.monitor->y + m_opts.monitor->h < geom->height) {
    h += geom->height - (m_opts.monitor->y + m_opts.monitor->h);
  } else if (m_opts.origin != edge::BOTTOM) {
    h += m_opts.monitor->y;
  }

  window win{m_connection, m_opts.window};
  win.reconfigure_struts(w, h, m_opts.pos.x, m_opts.origin == edge::BOTTOM);
}

/**
 * Reconfigure window wm hint values
 */
void bar::reconfigure_wm_hints() {
  m_log.trace("bar: Set window WM_NAME");
  xcb_icccm_set_wm_name(m_connection, m_opts.window, XCB_ATOM_STRING, 8, m_opts.wmname.size(), m_opts.wmname.c_str());
  xcb_icccm_set_wm_class(m_connection, m_opts.window, 15, "polybar\0Polybar");

  m_log.trace("bar: Set window _NET_Wm_opts.window_TYPE");
  set_wm_window_type(m_connection, m_opts.window, {_NET_WM_WINDOW_TYPE_DOCK});

  m_log.trace("bar: Set window _NET_WM_STATE");
  set_wm_state(m_connection, m_opts.window, {_NET_WM_STATE_STICKY, _NET_WM_STATE_ABOVE});

  m_log.trace("bar: Set window _NET_WM_DESKTOP");
  set_wm_desktop(m_connection, m_opts.window, 0xFFFFFFFF);

  m_log.trace("bar: Set window _NET_WM_PID");
  set_wm_pid(m_connection, m_opts.window, getpid());
}

/**
 * Broadcast current map state
 */
void bar::broadcast_visibility() {
  auto attr = m_connection.get_window_attributes(m_opts.window);

  if (attr->map_state == XCB_MAP_STATE_UNVIEWABLE) {
    m_sig.emit(visibility_change{move(false)});
  } else if (attr->map_state == XCB_MAP_STATE_UNMAPPED) {
    m_sig.emit(visibility_change{move(false)});
  } else {
    m_sig.emit(visibility_change{move(true)});
  }
}

/**
 * Event handler for XCB_BUTTON_PRESS events
 *
 * Used to map mouse clicks to bar actions
 */
void bar::handle(const evt::button_press& evt) {
  if (!m_mutex.try_lock()) {
    return;
  }

  std::lock_guard<std::mutex> guard(m_mutex, std::adopt_lock);

  if (m_buttonpress.deny(evt->time)) {
    return m_log.trace_x("bar: Ignoring button press (throttled)...");
  }

  m_log.trace("bar: Received button press: %i at pos(%i, %i)", evt->detail, evt->event_x, evt->event_y);

  const mousebtn button{static_cast<mousebtn>(evt->detail)};

  for (auto&& action : m_renderer->get_actions()) {
    if (action.active) {
      continue;
    } else if (action.button != button) {
      continue;
    } else if (action.start_x >= evt->event_x) {
      continue;
    } else if (action.end_x < evt->event_x) {
      continue;
    } else {
      m_log.trace("Found matching input area");
      m_sig.emit(button_press{string{action.command}});
      return;
    }
  }

  for (auto&& action : m_opts.actions) {
    if (action.button == button && !action.command.empty()) {
      m_log.trace("Triggering fallback click handler: %s", action.command);
      m_sig.emit(button_press{string{action.command}});
      return;
    }
  }

  m_log.warn("No matching input area found");
}

/**
 * Event handler for XCB_EXPOSE events
 *
 * Used to redraw the bar
 */
void bar::handle(const evt::expose& evt) {
  if (evt->window == m_opts.window && evt->count == 0) {
    if (m_tray->settings().running) {
      broadcast_visibility();
    }

    m_log.trace("bar: Received expose event");
    m_renderer->flush(false);
  }
}

/**
 * Event handler for XCB_PROPERTY_NOTIFY events
 *
 * - Emit events whenever the bar window's
 * visibility gets changed. This allows us to toggle the
 * state of the tray container even though the tray
 * window restacking failed.  Used as a fallback for
 * tedious WM's, like i3.
 *
 * - Track the root pixmap atom to update the
 * pseudo-transparent background when it changes
 */
void bar::handle(const evt::property_notify& evt) {
#ifdef VERBOSE_TRACELOG
  string atom_name = m_connection.get_atom_name(evt->atom).name();
  m_log.trace_x("bar: property_notify(%s)", atom_name);
#endif

  if (evt->window == m_opts.window && evt->atom == WM_STATE) {
    broadcast_visibility();
  }
}

POLYBAR_NS_END
