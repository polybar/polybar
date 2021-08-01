#include "components/bar.hpp"

#include <algorithm>

#include "components/config.hpp"
#include "components/renderer.hpp"
#include "components/screen.hpp"
#include "components/taskqueue.hpp"
#include "components/types.hpp"
#include "drawtypes/label.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "tags/dispatch.hpp"
#include "utils/bspwm.hpp"
#include "utils/color.hpp"
#include "utils/factory.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/ewmh.hpp"
#include "x11/extensions/all.hpp"
#include "x11/icccm.hpp"
#include "x11/tray_manager.hpp"

#if WITH_XCURSOR
#include "x11/cursor.hpp"
#endif

#if ENABLE_I3
#include "utils/i3.hpp"
#endif

POLYBAR_NS

using namespace signals::ui;

/**
 * Create instance
 */
bar::make_type bar::make(bool only_initialize_values) {
  auto action_ctxt = make_unique<tags::action_context>();

  // clang-format off
  return factory_util::unique<bar>(
        connection::make(),
        signal_emitter::make(),
        config::make(),
        logger::make(),
        screen::make(),
        tray_manager::make(),
        tags::dispatch::make(*action_ctxt),
        std::move(action_ctxt),
        taskqueue::make(),
        only_initialize_values);
  // clang-format on
}

/**
 * Construct bar instance
 *
 * TODO: Break out all tray handling
 */
bar::bar(connection& conn, signal_emitter& emitter, const config& config, const logger& logger,
    unique_ptr<screen>&& screen, unique_ptr<tray_manager>&& tray_manager, unique_ptr<tags::dispatch>&& dispatch,
    unique_ptr<tags::action_context>&& action_ctxt, unique_ptr<taskqueue>&& taskqueue, bool only_initialize_values)
    : m_connection(conn)
    , m_sig(emitter)
    , m_conf(config)
    , m_log(logger)
    , m_screen(forward<decltype(screen)>(screen))
    , m_tray(forward<decltype(tray_manager)>(tray_manager))
    , m_dispatch(forward<decltype(dispatch)>(dispatch))
    , m_action_ctxt(forward<decltype(action_ctxt)>(action_ctxt))
    , m_taskqueue(forward<decltype(taskqueue)>(taskqueue)) {
  string bs{m_conf.section()};

  // Get available RandR outputs
  auto monitor_name = m_conf.get(bs, "monitor", ""s);
  auto monitor_name_fallback = m_conf.get(bs, "monitor-fallback", ""s);
  m_opts.monitor_strict = m_conf.get(bs, "monitor-strict", m_opts.monitor_strict);
  m_opts.monitor_exact = m_conf.get(bs, "monitor-exact", m_opts.monitor_exact);
  auto monitors = randr_util::get_monitors(m_connection, m_connection.screen()->root, m_opts.monitor_strict, false);

  if (monitors.empty()) {
    throw application_error("No monitors found");
  }

  // if monitor_name is not defined, first check for primary monitor
  if (monitor_name.empty()) {
    for (auto&& mon : monitors) {
      if (mon->primary) {
        monitor_name = mon->name;
        break;
      }
    }
  }

  // if still not found (and not strict matching), get first connected monitor
  if (monitor_name.empty() && !m_opts.monitor_strict) {
    auto connected_monitors = randr_util::get_monitors(m_connection, m_connection.screen()->root, true, false);
    if (!connected_monitors.empty()) {
      monitor_name = connected_monitors[0]->name;
      m_log.warn("No monitor specified, using \"%s\"", monitor_name);
    }
  }

  // if still not found, get first monitor
  if (monitor_name.empty()) {
    monitor_name = monitors[0]->name;
    m_log.warn("No monitor specified, using \"%s\"", monitor_name);
  }

  // get the monitor data based on the name
  m_opts.monitor = randr_util::match_monitor(monitors, monitor_name, m_opts.monitor_exact);
  monitor_t fallback{};

  if (!monitor_name_fallback.empty()) {
    fallback = randr_util::match_monitor(monitors, monitor_name_fallback, m_opts.monitor_exact);
  }

  if (!m_opts.monitor) {
    if (fallback) {
      m_opts.monitor = move(fallback);
      m_log.warn("Monitor \"%s\" not found, reverting to fallback \"%s\"", monitor_name, m_opts.monitor->name);
    } else {
      throw application_error("Monitor \"" + monitor_name + "\" not found or disconnected");
    }
  }

  m_log.info("Loaded monitor %s (%ix%i+%i+%i)", m_opts.monitor->name, m_opts.monitor->w, m_opts.monitor->h,
      m_opts.monitor->x, m_opts.monitor->y);

  try {
    m_opts.override_redirect = m_conf.get<bool>(bs, "dock");
    m_conf.warn_deprecated(bs, "dock", "override-redirect");
  } catch (const key_error& err) {
    m_opts.override_redirect = m_conf.get(bs, "override-redirect", m_opts.override_redirect);
  }

  m_opts.dimvalue = m_conf.get(bs, "dim-value", 1.0);
  m_opts.dimvalue = math_util::cap(m_opts.dimvalue, 0.0, 1.0);

  m_opts.cursor_click = m_conf.get(bs, "cursor-click", ""s);
  m_opts.cursor_scroll = m_conf.get(bs, "cursor-scroll", ""s);
#if WITH_XCURSOR
  if (!m_opts.cursor_click.empty() && !cursor_util::valid(m_opts.cursor_click)) {
    m_log.warn("Ignoring unsupported cursor-click option '%s'", m_opts.cursor_click);
    m_opts.cursor_click.clear();
  }
  if (!m_opts.cursor_scroll.empty() && !cursor_util::valid(m_opts.cursor_scroll)) {
    m_log.warn("Ignoring unsupported cursor-scroll option '%s'", m_opts.cursor_scroll);
    m_opts.cursor_scroll.clear();
  }
#endif

  // Build WM_NAME
  m_opts.wmname = m_conf.get(bs, "wm-name", "polybar-" + bs.substr(4) + "_" + m_opts.monitor->name);
  m_opts.wmname = string_util::replace(m_opts.wmname, " ", "-");

  // Configure DPI
  {
    double dpi_x = 96, dpi_y = 96;
    if (m_conf.has(m_conf.section(), "dpi")) {
      dpi_x = dpi_y = m_conf.get<double>("dpi");
    } else {
      if (m_conf.has(m_conf.section(), "dpi-x")) {
        dpi_x = m_conf.get<double>("dpi-x");
      }
      if (m_conf.has(m_conf.section(), "dpi-y")) {
        dpi_y = m_conf.get<double>("dpi-y");
      }
    }

    // dpi to be computed
    if (dpi_x <= 0 || dpi_y <= 0) {
      auto screen = m_connection.screen();
      if (dpi_x <= 0) {
        dpi_x = screen->width_in_pixels * 25.4 / screen->width_in_millimeters;
      }
      if (dpi_y <= 0) {
        dpi_y = screen->height_in_pixels * 25.4 / screen->height_in_millimeters;
      }
    }

    m_opts.dpi_x = dpi_x;
    m_opts.dpi_y = dpi_y;

    m_log.info("Configured DPI = %gx%g", dpi_x, dpi_y);
  }

  // Load configuration values

  m_opts.origin = m_conf.get(bs, "bottom", false) ? edge::BOTTOM : edge::TOP;
  m_opts.spacing = m_conf.get(bs, "spacing", m_opts.spacing);
  m_opts.separator = drawtypes::load_optional_label(m_conf, bs, "separator", "");
  m_opts.locale = m_conf.get(bs, "locale", ""s);

  auto radius = m_conf.get<double>(bs, "radius", 0.0);
  auto top = m_conf.get(bs, "radius-top", radius);
  m_opts.radius.top_left = m_conf.get(bs, "radius-top-left", top);
  m_opts.radius.top_right = m_conf.get(bs, "radius-top-right", top);
  auto bottom = m_conf.get(bs, "radius-bottom", radius);
  m_opts.radius.bottom_left = m_conf.get(bs, "radius-bottom-left", bottom);
  m_opts.radius.bottom_right = m_conf.get(bs, "radius-bottom-right", bottom);

  auto padding = m_conf.get(bs, "padding", ZERO_SPACE);
  m_opts.padding.left = m_conf.get(bs, "padding-left", padding);
  m_opts.padding.right = m_conf.get(bs, "padding-right", padding);

  auto margin = m_conf.get(bs, "module-margin", ZERO_SPACE);
  m_opts.module_margin.left = m_conf.get(bs, "module-margin-left", margin);
  m_opts.module_margin.right = m_conf.get(bs, "module-margin-right", margin);

  if (only_initialize_values) {
    return;
  }

  // Load values used to adjust the struts atom
  m_opts.strut.top = m_conf.get("global/wm", "margin-top", 0U);
  m_opts.strut.bottom = m_conf.get("global/wm", "margin-bottom", 0U);

  // Load commands used for fallback click handlers
  vector<action> actions;
  actions.emplace_back(action{mousebtn::LEFT, m_conf.get(bs, "click-left", ""s)});
  actions.emplace_back(action{mousebtn::MIDDLE, m_conf.get(bs, "click-middle", ""s)});
  actions.emplace_back(action{mousebtn::RIGHT, m_conf.get(bs, "click-right", ""s)});
  actions.emplace_back(action{mousebtn::SCROLL_UP, m_conf.get(bs, "scroll-up", ""s)});
  actions.emplace_back(action{mousebtn::SCROLL_DOWN, m_conf.get(bs, "scroll-down", ""s)});
  actions.emplace_back(action{mousebtn::DOUBLE_LEFT, m_conf.get(bs, "double-click-left", ""s)});
  actions.emplace_back(action{mousebtn::DOUBLE_MIDDLE, m_conf.get(bs, "double-click-middle", ""s)});
  actions.emplace_back(action{mousebtn::DOUBLE_RIGHT, m_conf.get(bs, "double-click-right", ""s)});

  for (auto&& act : actions) {
    if (!act.command.empty()) {
      m_opts.actions.emplace_back(action{act.button, act.command});
    }
  }

  const auto parse_or_throw_color = [&](string key, rgba def) -> rgba {
    try {
      rgba color = m_conf.get(bs, key, def);

      /*
       * These are the base colors of the bar and cannot be alpha only
       * In that case, we just use the alpha channel on the default value.
       */
      return color.try_apply_alpha_to(def);
    } catch (const exception& err) {
      throw application_error(sstream() << "Failed to set " << key << " (reason: " << err.what() << ")");
    }
  };

  // Load background
  for (auto&& step : m_conf.get_list<rgba>(bs, "background", {})) {
    m_opts.background_steps.emplace_back(step);
  }

  if (!m_opts.background_steps.empty()) {
    m_opts.background = m_opts.background_steps[0];

    if (m_conf.has(bs, "background")) {
      m_log.warn("Ignoring `%s.background` (overridden by gradient background)", bs);
    }
  } else {
    m_opts.background = parse_or_throw_color("background", m_opts.background);
  }

  // Load foreground
  m_opts.foreground = parse_or_throw_color("foreground", m_opts.foreground);

  // Load over-/underline
  auto line_color = m_conf.get(bs, "line-color", rgba{0xFFFF0000});
  auto line_size = m_conf.get(bs, "line-size", ZERO_PX_EXTENT);

  auto overline_size = m_conf.get(bs, "overline-size", line_size);
  auto underline_size = m_conf.get(bs, "underline-size", line_size);

  m_opts.overline.size = unit_utils::extent_to_pixel<unsigned>(overline_size, m_opts.dpi_y);
  m_opts.overline.color = parse_or_throw_color("overline-color", line_color);
  m_opts.underline.size = unit_utils::extent_to_pixel<unsigned>(underline_size, m_opts.dpi_y);
  m_opts.underline.color = parse_or_throw_color("underline-color", line_color);

  // Load border settings
  auto border_color = m_conf.get(bs, "border-color", rgba{0x00000000});
  auto border_size = m_conf.get(bs, "border-size", percentage_with_offset{});
  auto border_top = m_conf.deprecated(bs, "border-top", "border-top-size", border_size);
  auto border_bottom = m_conf.deprecated(bs, "border-bottom", "border-bottom-size", border_size);
  auto border_left = m_conf.deprecated(bs, "border-left", "border-left-size", border_size);
  auto border_right = m_conf.deprecated(bs, "border-right", "border-right-size", border_size);

  m_opts.borders.emplace(edge::TOP, border_settings{});
  m_opts.borders[edge::TOP].size = percentage_with_offset_to_pixel(border_top, m_opts.monitor->h, m_opts.dpi_y);
  m_opts.borders[edge::TOP].color = parse_or_throw_color("border-top-color", border_color);
  m_opts.borders.emplace(edge::BOTTOM, border_settings{});
  m_opts.borders[edge::BOTTOM].size = percentage_with_offset_to_pixel(border_bottom, m_opts.monitor->h, m_opts.dpi_y);
  m_opts.borders[edge::BOTTOM].color = parse_or_throw_color("border-bottom-color", border_color);
  m_opts.borders.emplace(edge::LEFT, border_settings{});
  m_opts.borders[edge::LEFT].size = percentage_with_offset_to_pixel(border_left, m_opts.monitor->w, m_opts.dpi_x);
  m_opts.borders[edge::LEFT].color = parse_or_throw_color("border-left-color", border_color);
  m_opts.borders.emplace(edge::RIGHT, border_settings{});
  m_opts.borders[edge::RIGHT].size = percentage_with_offset_to_pixel(border_right, m_opts.monitor->w, m_opts.dpi_x);
  m_opts.borders[edge::RIGHT].color = parse_or_throw_color("border-right-color", border_color);

  // Load geometry values
  auto w = m_conf.get(m_conf.section(), "width", percentage_with_offset{100.});
  auto h = m_conf.get(m_conf.section(), "height", percentage_with_offset{0., {extent_type::PIXEL, 24}});
  auto offsetx = m_conf.get(m_conf.section(), "offset-x", percentage_with_offset{});
  auto offsety = m_conf.get(m_conf.section(), "offset-y", percentage_with_offset{});

  m_opts.size.w = percentage_with_offset_to_pixel(w, m_opts.monitor->w, m_opts.dpi_x);
  m_opts.size.h = percentage_with_offset_to_pixel(h, m_opts.monitor->h, m_opts.dpi_y);
  m_opts.offset.x = percentage_with_offset_to_pixel(offsetx, m_opts.monitor->w, m_opts.dpi_x);
  m_opts.offset.y = percentage_with_offset_to_pixel(offsety, m_opts.monitor->h, m_opts.dpi_y);

  // Apply offsets
  m_opts.pos.x = m_opts.offset.x + m_opts.monitor->x;
  m_opts.pos.y = m_opts.offset.y + m_opts.monitor->y;
  m_opts.size.h += m_opts.borders[edge::TOP].size;
  m_opts.size.h += m_opts.borders[edge::BOTTOM].size;

  if (m_opts.origin == edge::BOTTOM) {
    m_opts.pos.y = m_opts.monitor->y + m_opts.monitor->h - m_opts.size.h - m_opts.offset.y;
  }

  if (m_opts.size.w <= 0 || m_opts.size.w > m_opts.monitor->w) {
    throw application_error("Resulting bar width is out of bounds (" + to_string(m_opts.size.w) + ")");
  } else if (m_opts.size.h <= 0 || m_opts.size.h > m_opts.monitor->h) {
    throw application_error("Resulting bar height is out of bounds (" + to_string(m_opts.size.h) + ")");
  }

  m_log.info("Bar geometry: %ix%i+%i+%i; Borders: %d,%d,%d,%d", m_opts.size.w, m_opts.size.h, m_opts.pos.x,
      m_opts.pos.y, m_opts.borders[edge::TOP].size, m_opts.borders[edge::RIGHT].size, m_opts.borders[edge::BOTTOM].size,
      m_opts.borders[edge::LEFT].size);

  m_log.trace("bar: Attach X event sink");
  m_connection.attach_sink(this, SINK_PRIORITY_BAR);

  m_log.trace("bar: Attach signal receiver");
  m_sig.attach(this);
}

/**
 * Cleanup signal handlers and destroy the bar window
 */
bar::~bar() {
  std::lock_guard<std::mutex> guard(m_mutex);
  m_connection.detach_sink(this, SINK_PRIORITY_BAR);
  m_sig.detach(this);
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
 * \param data Input string
 * \param force Unless true, do not parse unchanged data
 */
void bar::parse(string&& data, bool force) {
  if (!m_mutex.try_lock()) {
    return;
  }

  std::lock_guard<std::mutex> guard(m_mutex, std::adopt_lock);

  bool unchanged = data == m_lastinput;

  m_lastinput = data;

  if (force) {
    m_log.trace("bar: Force update");
  } else if (!m_visible) {
    return m_log.trace("bar: Ignoring update (invisible)");
  } else if (m_opts.shaded) {
    return m_log.trace("bar: Ignoring update (shaded)");
  } else if (unchanged) {
    return m_log.trace("bar: Ignoring update (unchanged)");
  }

  auto rect = m_opts.inner_area();

  if (m_tray && !m_tray->settings().detached && m_tray->settings().configured_slots) {
    auto trayalign = m_tray->settings().align;
    auto traywidth = m_tray->settings().configured_w;
    if (trayalign == alignment::LEFT) {
      rect.x += traywidth;
      rect.width -= traywidth;
    } else if (trayalign == alignment::RIGHT) {
      rect.width -= traywidth;
    }
  }

  m_log.info("Redrawing bar window");
  m_renderer->begin(rect);

  try {
    m_dispatch->parse(settings(), *m_renderer, std::move(data));
  } catch (const exception& err) {
    m_log.err("Failed to parse contents (reason: %s)", err.what());
  }

  m_renderer->end();

  const auto check_dblclicks = [&]() -> bool {
    if (m_action_ctxt->has_double_click()) {
      return true;
    }

    for (auto&& action : m_opts.actions) {
      if (static_cast<int>(action.button) >= static_cast<int>(mousebtn::DOUBLE_LEFT)) {
        return true;
      }
    }
    return false;
  };
  m_dblclicks = check_dblclicks();
}

/**
 * Hide the bar by unmapping its X window
 */
void bar::hide() {
  if (!m_visible) {
    return;
  }

  try {
    m_log.info("Hiding bar window");
    m_sig.emit(visibility_change{false});
    m_connection.unmap_window_checked(m_opts.window);
    m_connection.flush();
    m_visible = false;
  } catch (const exception& err) {
    m_log.err("Failed to unmap bar window (err=%s", err.what());
  }
}

/**
 * Show the bar by mapping its X window and
 * trigger a redraw of previous content
 */
void bar::show() {
  if (m_visible) {
    return;
  }

  try {
    m_log.info("Showing bar window");
    m_sig.emit(visibility_change{true});
    /**
     * First reconfigures the window so that WMs that discard some information
     * when unmapping have the correct window properties (geometry etc).
     */
    reconfigue_window();
    m_connection.map_window_checked(m_opts.window);
    m_connection.flush();
    m_visible = true;
    parse(string{m_lastinput}, true);
  } catch (const exception& err) {
    m_log.err("Failed to map bar window (err=%s", err.what());
  }
}

/**
 * Toggle the bar's visibility state
 */
void bar::toggle() {
  if (m_visible) {
    hide();
  } else {
    show();
  }
}

/**
 * Move the bar window above defined sibling
 * in the X window stack
 */
void bar::restack_window() {
  string wm_restack;

  try {
    wm_restack = m_conf.get(m_conf.section(), "wm-restack");
  } catch (const key_error& err) {
    return;
  }

  auto restacked = false;

  if (wm_restack == "generic") {
    try {
      auto children = m_connection.query_tree(m_connection.screen()->root).children();
      if (children.begin() != children.end() && *children.begin() != m_opts.window) {
        const unsigned int value_mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
        const unsigned int value_list[2]{*children.begin(), XCB_STACK_MODE_BELOW};
        m_connection.configure_window_checked(m_opts.window, value_mask, value_list);
      }
      restacked = true;
    } catch (const exception& err) {
      m_log.err("Failed to restack bar window (err=%s)", err.what());
    }
  } else if (wm_restack == "bspwm") {
    restacked = bspwm_util::restack_to_root(m_connection, m_opts.monitor, m_opts.window);
#if ENABLE_I3
  } else if (wm_restack == "i3" && m_opts.override_redirect) {
    restacked = i3_util::restack_to_root(m_connection, m_opts.window);
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

void bar::reconfigue_window() {
  m_log.trace("bar: Reconfigure window");
  restack_window();
  reconfigure_geom();
  reconfigure_struts();
  reconfigure_wm_hints();
}

/**
 * Reconfigure window geometry
 */
void bar::reconfigure_geom() {
  window win{m_connection, m_opts.window};
  win.reconfigure_geom(m_opts.size.w, m_opts.size.h, m_opts.pos.x, m_opts.pos.y);
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
  const auto& win = m_opts.window;

  m_log.trace("bar: Set window WM_NAME");
  icccm_util::set_wm_name(m_connection, win, m_opts.wmname.c_str(), m_opts.wmname.size(), "polybar\0Polybar", 15_z);

  m_log.trace("bar: Set window _NET_WM_WINDOW_TYPE");
  ewmh_util::set_wm_window_type(win, {_NET_WM_WINDOW_TYPE_DOCK});

  m_log.trace("bar: Set window _NET_WM_STATE");
  ewmh_util::set_wm_state(win, {_NET_WM_STATE_STICKY, _NET_WM_STATE_ABOVE});

  m_log.trace("bar: Set window _NET_WM_DESKTOP");
  ewmh_util::set_wm_desktop(win, 0xFFFFFFFF);

  m_log.trace("bar: Set window _NET_WM_PID");
  ewmh_util::set_wm_pid(win);
}

/**
 * Broadcast current map state
 */
void bar::broadcast_visibility() {
  auto attr = m_connection.get_window_attributes(m_opts.window);

  if (attr->map_state == XCB_MAP_STATE_UNVIEWABLE) {
    m_sig.emit(visibility_change{false});
  } else if (attr->map_state == XCB_MAP_STATE_UNMAPPED) {
    m_sig.emit(visibility_change{false});
  } else {
    m_sig.emit(visibility_change{true});
  }
}

/**
 * Event handler for XCB_DESTROY_NOTIFY events
 */
void bar::handle(const evt::client_message& evt) {
  if (evt->type == WM_PROTOCOLS && evt->data.data32[0] == WM_DELETE_WINDOW && evt->window == m_opts.window) {
    m_log.err("Bar window has been destroyed, shutting down...");
    m_connection.disconnect();
  }
}

/**
 * Event handler for XCB_DESTROY_NOTIFY events
 */
void bar::handle(const evt::destroy_notify& evt) {
  if (evt->window == m_opts.window) {
    m_connection.disconnect();
  }
}

/**
 * Event handler for XCB_ENTER_NOTIFY events
 *
 * Used to brighten the window by setting the
 * _NET_WM_WINDOW_OPACITY atom value
 */
void bar::handle(const evt::enter_notify&) {
#if 0
#ifdef DEBUG_SHADED
  if (m_opts.origin == edge::TOP) {
    m_taskqueue->defer_unique("window-hover", 25ms, [&](size_t) { m_sig.emit(signals::ui::unshade_window{}); });
    return;
  }
#endif
#endif
  if (m_opts.dimmed) {
    m_taskqueue->defer_unique("window-dim", 25ms, [&](size_t) {
      m_opts.dimmed = false;
      m_sig.emit(dim_window{1.0});
    });
  } else if (m_taskqueue->exist("window-dim")) {
    m_taskqueue->purge("window-dim");
  }
}

/**
 * Event handler for XCB_LEAVE_NOTIFY events
 *
 * Used to dim the window by setting the
 * _NET_WM_WINDOW_OPACITY atom value
 */
void bar::handle(const evt::leave_notify&) {
#if 0
#ifdef DEBUG_SHADED
  if (m_opts.origin == edge::TOP) {
    m_taskqueue->defer_unique("window-hover", 25ms, [&](size_t) { m_sig.emit(signals::ui::shade_window{}); });
    return;
  }
#endif
#endif
  if (!m_opts.dimmed) {
    m_taskqueue->defer_unique("window-dim", 3s, [&](size_t) {
      m_opts.dimmed = true;
      m_sig.emit(dim_window{double(m_opts.dimvalue)});
    });
  }
}

/**
 * Event handler for XCB_MOTION_NOTIFY events
 *
 * Used to change the cursor depending on the module
 */
void bar::handle(const evt::motion_notify& evt) {
  if (!m_mutex.try_lock()) {
    return;
  }

  std::lock_guard<std::mutex> guard(m_mutex, std::adopt_lock);

  m_log.trace("bar: Detected motion: %i at pos(%i, %i)", evt->detail, evt->event_x, evt->event_y);
#if WITH_XCURSOR
  m_motion_pos = evt->event_x;
  // scroll cursor is less important than click cursor, so we shouldn't return until we are sure there is no click
  // action
  bool found_scroll = false;
  const auto has_action = [&](const vector<mousebtn>& buttons) -> bool {
    for (auto btn : buttons) {
      if (m_action_ctxt->has_action(btn, m_motion_pos) != tags::NO_ACTION) {
        return true;
      }
    }

    return false;
  };

  if (has_action({mousebtn::LEFT, mousebtn::MIDDLE, mousebtn::RIGHT, mousebtn::DOUBLE_LEFT, mousebtn::DOUBLE_MIDDLE,
          mousebtn::DOUBLE_RIGHT})) {
    if (!string_util::compare(m_opts.cursor, m_opts.cursor_click)) {
      m_opts.cursor = m_opts.cursor_click;
      m_sig.emit(cursor_change{string{m_opts.cursor}});
    }

    return;
  }

  if (has_action({mousebtn::SCROLL_DOWN, mousebtn::SCROLL_UP})) {
    if (!string_util::compare(m_opts.cursor, m_opts.cursor_scroll)) {
      m_opts.cursor = m_opts.cursor_scroll;
      m_sig.emit(cursor_change{string{m_opts.cursor}});
    }
    return;
  }

  const auto find_click_area = [&](const action& action) {
    if (!m_opts.cursor_click.empty() &&
        !(action.button == mousebtn::SCROLL_UP || action.button == mousebtn::SCROLL_DOWN ||
            action.button == mousebtn::NONE)) {
      if (!string_util::compare(m_opts.cursor, m_opts.cursor_click)) {
        m_opts.cursor = m_opts.cursor_click;
        m_sig.emit(cursor_change{string{m_opts.cursor}});
      }
      return true;
    } else if (!m_opts.cursor_scroll.empty() &&
               (action.button == mousebtn::SCROLL_UP || action.button == mousebtn::SCROLL_DOWN)) {
      if (!found_scroll) {
        found_scroll = true;
      }
    }
    return false;
  };

  for (auto&& action : m_opts.actions) {
    if (!action.command.empty()) {
      m_log.trace("Found matching fallback handler");
      if (find_click_area(action))
        return;
    }
  }
  if (found_scroll) {
    if (!string_util::compare(m_opts.cursor, m_opts.cursor_scroll)) {
      m_opts.cursor = m_opts.cursor_scroll;
      m_sig.emit(cursor_change{string{m_opts.cursor}});
    }
    return;
  }
  if (!string_util::compare(m_opts.cursor, "default")) {
    m_log.trace("No matching cursor area found");
    m_opts.cursor = "default";
    m_sig.emit(cursor_change{string{m_opts.cursor}});
    return;
  }
#endif
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

  m_buttonpress_btn = static_cast<mousebtn>(evt->detail);
  m_buttonpress_pos = evt->event_x;

  const auto deferred_fn = [&](size_t) {
    tags::action_t action = m_action_ctxt->has_action(m_buttonpress_btn, m_buttonpress_pos);

    if (action != tags::NO_ACTION) {
      m_log.trace("Found matching input area");
      m_sig.emit(button_press{m_action_ctxt->get_action(action)});
      return;
    }

    for (auto&& action : m_opts.actions) {
      if (action.button == m_buttonpress_btn && !action.command.empty()) {
        m_log.trace("Found matching fallback handler");
        m_sig.emit(button_press{string{action.command}});
        return;
      }
    }
    m_log.info("No matching input area found (btn=%i)", static_cast<int>(m_buttonpress_btn));
  };

  const auto check_double = [&](string&& id, mousebtn&& btn) {
    if (!m_taskqueue->exist(id)) {
      m_doubleclick.event = evt->time;
      m_taskqueue->defer(id, taskqueue::deferred::duration{m_doubleclick.offset}, deferred_fn);
    } else if (m_doubleclick.deny(evt->time)) {
      m_doubleclick.event = 0;
      m_buttonpress_btn = btn;
      m_taskqueue->defer_unique(id, 0ms, deferred_fn);
    }
  };

  // If there are no double click handlers defined we can
  // just by-pass the click timer handling
  if (!m_dblclicks) {
    deferred_fn(0);
  } else if (evt->detail == static_cast<int>(mousebtn::LEFT)) {
    check_double("buttonpress-left", mousebtn::DOUBLE_LEFT);
  } else if (evt->detail == static_cast<int>(mousebtn::MIDDLE)) {
    check_double("buttonpress-middle", mousebtn::DOUBLE_MIDDLE);
  } else if (evt->detail == static_cast<int>(mousebtn::RIGHT)) {
    check_double("buttonpress-right", mousebtn::DOUBLE_RIGHT);
  } else {
    deferred_fn(0);
  }
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
    m_renderer->flush();
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
 */
void bar::handle(const evt::property_notify& evt) {
#ifdef DEBUG_LOGGER_VERBOSE
  string atom_name = m_connection.get_atom_name(evt->atom).name();
  m_log.trace_x("bar: property_notify(%s)", atom_name);
#endif

  if (evt->window == m_opts.window && evt->atom == WM_STATE) {
    broadcast_visibility();
  }
}

void bar::handle(const evt::configure_notify&) {
  // The absolute position of the window in the root may be different after configuration is done
  // (for example, because the parent is not positioned at 0/0 in the root window).
  // Notify components that the geometry may have changed (used by the background manager for example).
  m_sig.emit(signals::ui::update_geometry{});
}

bool bar::on(const signals::eventqueue::start&) {
  m_log.trace("bar: Create renderer");
  m_renderer = renderer::make(m_opts, *m_action_ctxt);
  m_opts.window = m_renderer->window();

  // Subscribe to window enter and leave events
  // if we should dim the window
  if (m_opts.dimvalue != 1.0) {
    m_connection.ensure_event_mask(m_opts.window, XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW);
  }
  if (!m_opts.cursor_click.empty() || !m_opts.cursor_scroll.empty()) {
    m_connection.ensure_event_mask(m_opts.window, XCB_EVENT_MASK_POINTER_MOTION);
  }
  m_connection.ensure_event_mask(m_opts.window, XCB_EVENT_MASK_STRUCTURE_NOTIFY);

  m_log.info("Bar window: %s", m_connection.id(m_opts.window));
  reconfigue_window();

  m_log.trace("bar: Map window");
  m_connection.map_window_checked(m_opts.window);

  // With the mapping, the absolute position of our window may have changed (due to re-parenting for example).
  // Notify all components that depend on the absolute bar position (such as the background manager).
  m_sig.emit(signals::ui::update_geometry{});

  // Reconfigure window position after mapping (required by Openbox)
  reconfigure_pos();

  m_log.trace("bar: Draw empty bar");
  m_renderer->begin(m_opts.inner_area());
  m_renderer->end();

  m_sig.emit(signals::ui::ready{});

  // TODO: tray manager could run this internally on ready event
  m_log.trace("bar: Setup tray manager");
  m_tray->setup(static_cast<const bar_settings&>(m_opts));

  broadcast_visibility();

  return true;
}

bool bar::on(const signals::ui::unshade_window&) {
  m_opts.shaded = false;
  m_opts.shade_size.w = m_opts.size.w;
  m_opts.shade_size.h = m_opts.size.h;
  m_opts.shade_pos.x = m_opts.pos.x;
  m_opts.shade_pos.y = m_opts.pos.y;

  double distance{static_cast<double>(m_opts.shade_size.h - m_connection.get_geometry(m_opts.window)->height)};
  double steptime{25.0 / 2.0};
  m_anim_step = distance / steptime / 2.0;

  m_taskqueue->defer_unique(
      "window-shade", 25ms,
      [&](size_t remaining) {
        if (!m_opts.shaded) {
          m_sig.emit(signals::ui::tick{});
        }
        if (!remaining) {
          m_renderer->flush();
        }
        if (m_opts.dimmed) {
          m_opts.dimmed = false;
          m_sig.emit(dim_window{1.0});
        }
      },
      taskqueue::deferred::duration{25ms}, 10U);

  return true;
}

bool bar::on(const signals::ui::shade_window&) {
  taskqueue::deferred::duration offset{2000ms};

  if (!m_opts.shaded && m_opts.shade_size.h != m_opts.size.h) {
    offset = taskqueue::deferred::duration{25ms};
  }

  m_opts.shaded = true;
  m_opts.shade_size.h = 5;
  m_opts.shade_size.w = m_opts.size.w;
  m_opts.shade_pos.x = m_opts.pos.x;
  m_opts.shade_pos.y = m_opts.pos.y;

  if (m_opts.origin == edge::BOTTOM) {
    m_opts.shade_pos.y = m_opts.pos.y + m_opts.size.h - m_opts.shade_size.h;
  }

  double distance{static_cast<double>(m_connection.get_geometry(m_opts.window)->height - m_opts.shade_size.h)};
  double steptime{25.0 / 2.0};
  m_anim_step = distance / steptime / 2.0;

  m_taskqueue->defer_unique(
      "window-shade", 25ms,
      [&](size_t remaining) {
        if (m_opts.shaded) {
          m_sig.emit(signals::ui::tick{});
        }
        if (!remaining) {
          m_renderer->flush();
        }
        if (!m_opts.dimmed) {
          m_opts.dimmed = true;
          m_sig.emit(dim_window{double{m_opts.dimvalue}});
        }
      },
      move(offset), 10U);

  return true;
}

bool bar::on(const signals::ui::tick&) {
  auto geom = m_connection.get_geometry(m_opts.window);
  if (geom->y == m_opts.shade_pos.y && geom->height == m_opts.shade_size.h) {
    return false;
  }

  unsigned int mask{0};
  unsigned int values[7]{0};
  xcb_params_configure_window_t params{};

  if (m_opts.shade_size.h > geom->height) {
    XCB_AUX_ADD_PARAM(&mask, &params, height, static_cast<unsigned int>(geom->height + m_anim_step));
    params.height = std::max(1U, std::min(params.height, static_cast<unsigned int>(m_opts.shade_size.h)));
  } else if (m_opts.shade_size.h < geom->height) {
    XCB_AUX_ADD_PARAM(&mask, &params, height, static_cast<unsigned int>(geom->height - m_anim_step));
    params.height = std::max(1U, std::max(params.height, static_cast<unsigned int>(m_opts.shade_size.h)));
  }

  if (m_opts.shade_pos.y > geom->y) {
    XCB_AUX_ADD_PARAM(&mask, &params, y, static_cast<int>(geom->y + m_anim_step));
    params.y = std::min(params.y, static_cast<int>(m_opts.shade_pos.y));
  } else if (m_opts.shade_pos.y < geom->y) {
    XCB_AUX_ADD_PARAM(&mask, &params, y, static_cast<int>(geom->y - m_anim_step));
    params.y = std::max(params.y, static_cast<int>(m_opts.shade_pos.y));
  }

  connection::pack_values(mask, &params, values);

  m_connection.configure_window(m_opts.window, mask, values);
  m_connection.flush();

  return false;
}

bool bar::on(const signals::ui::dim_window& sig) {
  m_opts.dimmed = sig.cast() != 1.0;
  ewmh_util::set_wm_window_opacity(m_opts.window, sig.cast() * 0xFFFFFFFF);
  return false;
}

#if WITH_XCURSOR
bool bar::on(const signals::ui::cursor_change& sig) {
  if (!cursor_util::set_cursor(m_connection, m_connection.screen(), m_opts.window, sig.cast())) {
    m_log.warn("Failed to create cursor context");
  }
  m_connection.flush();
  return false;
}
#endif

POLYBAR_NS_END
