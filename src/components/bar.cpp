#include <xcb/xcb_icccm.h>

#include "components/bar.hpp"
#include "components/parser.hpp"
#include "components/renderer.hpp"
#include "components/screen.hpp"
#include "components/signals.hpp"
#include "utils/bspwm.hpp"
#include "utils/color.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/ewmh.hpp"
#include "x11/fonts.hpp"
#include "x11/graphics.hpp"
#include "x11/tray.hpp"
#include "x11/wm.hpp"
#include "x11/xutils.hpp"

#if ENABLE_I3
#include "utils/i3.hpp"
#endif

POLYBAR_NS

namespace ph = std::placeholders;

/**
 * Configure injection module
 */
di::injector<unique_ptr<bar>> configure_bar() {
  // clang-format off
  return di::make_injector(
      configure_connection(),
      configure_config(),
      configure_logger(),
      configure_screen(),
      configure_tray_manager());
  // clang-format on
}

/**
 * Construct bar instance
 */
bar::bar(connection& conn, const config& config, const logger& logger, unique_ptr<screen> screen,
    unique_ptr<tray_manager> tray_manager)
    : m_connection(conn), m_conf(config), m_log(logger), m_screen(move(screen)), m_tray(move(tray_manager)) {}

/**
 * Cleanup signal handlers and destroy the bar window
 */
bar::~bar() {
  std::lock_guard<std::mutex> guard(m_mutex);
  m_connection.detach_sink(this, SINK_PRIORITY_BAR);
  m_tray.reset();
}

/**
 * Create required components
 *
 * This is done outside the constructor due to boost::di noexcept
 */
void bar::bootstrap(bool nodraw) {
  auto bs = m_conf.bar_section();

  setup_monitor();

  m_log.trace("bar: Load config values");
  {
    m_opts.locale = m_conf.get<string>(bs, "locale", "");
    m_opts.separator = string_util::trim(m_conf.get<string>(bs, "separator", ""), '"');
    m_opts.wmname = m_conf.get<string>(bs, "wm-name", "polybar-" + bs.substr(4) + "_" + m_opts.monitor->name);
    m_opts.wmname = string_util::replace(m_opts.wmname, " ", "-");

    if (m_conf.get<bool>(bs, "bottom", false)) {
      m_opts.origin = edge::BOTTOM;
    }

    try {
      m_opts.override_redirect = m_conf.get<bool>(bs, "dock");
      m_conf.warn_deprecated(bs, "dock", "override-redirect");
    } catch (const key_error& err) {
      m_opts.override_redirect = m_conf.get<bool>(bs, "override-redirect", m_opts.override_redirect);
    }

    GET_CONFIG_VALUE(bs, m_opts.spacing, "spacing");
    GET_CONFIG_VALUE(bs, m_opts.padding.left, "padding-left");
    GET_CONFIG_VALUE(bs, m_opts.padding.right, "padding-right");
    GET_CONFIG_VALUE(bs, m_opts.module_margin.left, "module-margin-left");
    GET_CONFIG_VALUE(bs, m_opts.module_margin.right, "module-margin-right");

    m_opts.strut.top = m_conf.get<int>("global/wm", "margin-top", 0);
    m_opts.strut.bottom = m_conf.get<int>("global/wm", "margin-bottom", 0);

    m_buttonpress.offset = xutils::event_timer_ms(m_conf, xcb_button_press_event_t{});

    // Get fallback click handlers
    auto click_left = m_conf.get<string>(bs, "click-left", "");
    auto click_middle = m_conf.get<string>(bs, "click-middle", "");
    auto click_right = m_conf.get<string>(bs, "click-right", "");
    auto scroll_up = m_conf.get<string>(bs, "scroll-up", "");
    auto scroll_down = m_conf.get<string>(bs, "scroll-down", "");

    if (!click_left.empty()) {
      m_opts.actions.emplace_back(action{mousebtn::LEFT, move(click_left)});
    }
    if (!click_middle.empty()) {
      m_opts.actions.emplace_back(action{mousebtn::MIDDLE, move(click_middle)});
    }
    if (!click_right.empty()) {
      m_opts.actions.emplace_back(action{mousebtn::RIGHT, move(click_right)});
    }
    if (!scroll_up.empty()) {
      m_opts.actions.emplace_back(action{mousebtn::SCROLL_UP, move(scroll_up)});
    }
    if (!scroll_down.empty()) {
      m_opts.actions.emplace_back(action{mousebtn::SCROLL_DOWN, move(scroll_down)});
    }
  }

  m_log.trace("bar: Load color values");
  {
    m_opts.background =
        color::parse(m_conf.get<string>(bs, "background", color_util::hex<uint16_t>(m_opts.background)));
    m_opts.foreground =
        color::parse(m_conf.get<string>(bs, "foreground", color_util::hex<uint16_t>(m_opts.foreground)));

    auto linecolor = color::parse(m_conf.get<string>(bs, "linecolor", "#f00"));
    auto lineheight = m_conf.get<int>(bs, "lineheight", 0);

    m_conf.warn_deprecated(bs, "linecolor", "{underline,overline}-color");
    m_conf.warn_deprecated(bs, "lineheight", "{underline,overline}-size");

    try {
      m_opts.overline.size = m_conf.get<int16_t>(bs, "overline-size", lineheight);
      m_opts.overline.color = color::parse(m_conf.get<string>(bs, "overline-color"));
    } catch (const key_error& err) {
      m_opts.overline.color = linecolor;
    }

    try {
      m_opts.underline.size = m_conf.get<uint16_t>(bs, "underline-size", lineheight);
      m_opts.underline.color = color::parse(m_conf.get<string>(bs, "underline-color"));
    } catch (const key_error& err) {
      m_opts.underline.color = linecolor;
    }
  }

  m_log.trace("bar: Load border values");
  {
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
  }

  if (nodraw) {
    m_log.trace("bar: Abort bootstrap routine (reason: nodraw)");
    m_tray.reset();
    return;
  }

  m_log.trace("bar: Attaching sink to registry");
  m_connection.attach_sink(this, SINK_PRIORITY_BAR);

  configure_geom();

  m_renderer = configure_renderer(m_opts, m_conf.get_list<string>(bs, "font", {})).create<unique_ptr<renderer>>();
  m_window = m_renderer->window();

  m_log.info("Bar window: %s", m_connection.id(m_window));

  restack_window();

  m_log.trace("bar: Reconfigure window");
  reconfigure_struts();
  reconfigure_wm_hints();

  m_log.trace("bar: Map window");
  m_connection.map_window_checked(m_window);

  // Reconfigure window position after mapping (required by Openbox)
  // Required by Openbox
  reconfigure_pos();

  m_log.trace("bar: Attach parser signal handlers");
  g_signals::parser::background_change = bind(&renderer::set_background, m_renderer.get(), ph::_1);
  g_signals::parser::foreground_change = bind(&renderer::set_foreground, m_renderer.get(), ph::_1);
  g_signals::parser::underline_change = bind(&renderer::set_underline, m_renderer.get(), ph::_1);
  g_signals::parser::overline_change = bind(&renderer::set_overline, m_renderer.get(), ph::_1);
  g_signals::parser::pixel_offset = bind(&renderer::fill_shift, m_renderer.get(), ph::_1);
  g_signals::parser::alignment_change = bind(&renderer::set_alignment, m_renderer.get(), ph::_1);
  g_signals::parser::attribute_set = bind(&renderer::set_attribute, m_renderer.get(), ph::_1, true);
  g_signals::parser::attribute_unset = bind(&renderer::set_attribute, m_renderer.get(), ph::_1, false);
  g_signals::parser::attribute_toggle = bind(&renderer::toggle_attribute, m_renderer.get(), ph::_1);
  g_signals::parser::action_block_open = bind(&renderer::begin_action, m_renderer.get(), ph::_1, ph::_2);
  g_signals::parser::action_block_close = bind(&renderer::end_action, m_renderer.get(), ph::_1);
  g_signals::parser::font_change = bind(&renderer::set_fontindex, m_renderer.get(), ph::_1);
  g_signals::parser::ascii_text_write = bind(&renderer::draw_character, m_renderer.get(), ph::_1);
  g_signals::parser::unicode_text_write = bind(&renderer::draw_character, m_renderer.get(), ph::_1);
  g_signals::parser::string_write = bind(&renderer::draw_textstring, m_renderer.get(), ph::_1, ph::_2);

  try {
    m_log.trace("bar: Drawing empty bar");
    m_renderer->begin();
    m_renderer->fill_background();
    m_renderer->end();
  } catch (const exception& err) {
    throw application_error("Failed to output empty bar window (reason: " + string{err.what()} + ")");
  }
}

/**
 * Setup tray manager
 */
void bar::bootstrap_tray() {
  if (!m_tray) {
    return;
  }

  tray_settings settings;

  auto bs = m_conf.bar_section();
  auto tray_position = m_conf.get<string>(bs, "tray-position", "");

  if (tray_position == "left") {
    settings.align = alignment::LEFT;
  } else if (tray_position == "right") {
    settings.align = alignment::RIGHT;
  } else if (tray_position == "center") {
    settings.align = alignment::CENTER;
  } else {
    settings.align = alignment::NONE;
  }

  if (settings.align == alignment::NONE) {
    m_log.warn("Disabling tray manager (reason: disabled in config)");
    m_tray.reset();
    return;
  }

  settings.detached = m_conf.get<bool>(bs, "tray-detached", false);

  settings.height = m_opts.size.h;
  settings.height -= m_opts.borders.at(edge::BOTTOM).size;
  settings.height -= m_opts.borders.at(edge::TOP).size;
  settings.height_fill = settings.height;

  if (settings.height % 2 != 0) {
    settings.height--;
  }

  auto maxsize = m_conf.get<int>(bs, "tray-maxsize", 16);
  if (settings.height > maxsize) {
    settings.spacing += (settings.height - maxsize) / 2;
    settings.height = maxsize;
  }

  settings.width_max = m_opts.size.w;
  settings.width = settings.height;
  settings.orig_y = m_opts.pos.y + m_opts.borders.at(edge::TOP).size;

  // Apply user-defined scaling
  auto scale = m_conf.get<float>(bs, "tray-scale", 1.0);
  settings.width *= scale;
  settings.height_fill *= scale;

  auto inner_area = m_opts.inner_area(true);

  switch (settings.align) {
    case alignment::NONE:
      break;
    case alignment::LEFT:
      settings.orig_x = inner_area.x;
      break;
    case alignment::CENTER:
      settings.orig_x = inner_area.x + inner_area.width / 2 - settings.width / 2;
      break;
    case alignment::RIGHT:
      settings.orig_x = inner_area.x + inner_area.width;
      break;
  }

  // Set user-defined background color
  if (!(settings.transparent = m_conf.get<bool>(bs, "tray-transparent", settings.transparent))) {
    auto bg = m_conf.get<string>(bs, "tray-background", "");

    if (bg.length() > 7) {
      m_log.warn("Alpha support for the systray is limited. See the wiki for more details.");
    }

    if (!bg.empty()) {
      settings.background = color::parse(bg, g_colorempty);
    } else {
      settings.background = m_opts.background;
    }

    if (color_util::alpha_channel(settings.background) == 0) {
      settings.transparent = true;
      settings.background = 0;
    }
  }

  // Add user-defined padding
  settings.spacing += m_conf.get<int>(bs, "tray-padding", 0);

  // Add user-defiend offset
  auto offset_x_def = m_conf.get<string>(bs, "tray-offset-x", "");
  auto offset_y_def = m_conf.get<string>(bs, "tray-offset-y", "");

  auto offset_x = atoi(offset_x_def.c_str());
  auto offset_y = atoi(offset_y_def.c_str());

  if (offset_x != 0 && offset_x_def.find('%') != string::npos) {
    offset_x = math_util::percentage_to_value<int>(offset_x, m_opts.monitor->w);
    offset_x -= settings.width / 2;
  }

  if (offset_y != 0 && offset_y_def.find('%') != string::npos) {
    offset_y = math_util::percentage_to_value<int>(offset_y, m_opts.monitor->h);
    offset_y -= settings.width / 2;
  }

  settings.orig_x += offset_x;
  settings.orig_y += offset_y;

  // Put the tray next to the bar in the window stack
  settings.sibling = m_window;

  try {
    m_log.trace("bar: Setup tray manager");
    m_tray->bootstrap(settings);
  } catch (const exception& err) {
    m_log.err(err.what());
    m_log.warn("Failed to setup tray, disabling...");
    m_tray.reset();
  }
}

/**
 * Activate tray manager
 */
void bar::activate_tray() {
  if (!m_tray) {
    return;
  }

  m_log.trace("bar: Activate tray manager");

  try {
    broadcast_visibility();
    m_tray->activate();
  } catch (const exception& err) {
    m_log.err(err.what());
    m_log.err("Failed to activate tray manager, disabling...");
    m_tray.reset();
  }
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
    parser parser{m_log, m_opts};
    parser(data);
  } catch (const parser_error& err) {
    m_log.err("Failed to parse contents (reason: %s)", err.what());
  }

  m_renderer->end();
}

/**
 * Configure geometry values
 */
void bar::configure_geom() {
  m_log.trace("bar: Configure window geometry");

  auto w = m_conf.get<string>(m_conf.bar_section(), "width", "100%");
  auto h = m_conf.get<string>(m_conf.bar_section(), "height", "24");

  auto offsetx = m_conf.get<string>(m_conf.bar_section(), "offset-x", "");
  auto offsety = m_conf.get<string>(m_conf.bar_section(), "offset-y", "");

  // look for user-defined width
  if ((m_opts.size.w = atoi(w.c_str())) && w.find('%') != string::npos) {
    m_opts.size.w = math_util::percentage_to_value<int>(m_opts.size.w, m_opts.monitor->w);
  }

  // look for user-defined  height
  if ((m_opts.size.h = atoi(h.c_str())) && h.find('%') != string::npos) {
    m_opts.size.h = math_util::percentage_to_value<int>(m_opts.size.h, m_opts.monitor->h);
  }

  // look for user-defined offset-x
  if ((m_opts.offset.x = atoi(offsetx.c_str())) != 0 && offsetx.find('%') != string::npos) {
    m_opts.offset.x = math_util::percentage_to_value<int>(m_opts.offset.x, m_opts.monitor->w);
  }

  // look for user-defined offset-y
  if ((m_opts.offset.y = atoi(offsety.c_str())) != 0 && offsety.find('%') != string::npos) {
    m_opts.offset.y = math_util::percentage_to_value<int>(m_opts.offset.y, m_opts.monitor->h);
  }

  // apply offsets
  m_opts.pos.x = m_opts.offset.x + m_opts.monitor->x;
  m_opts.pos.y = m_opts.offset.y + m_opts.monitor->y;

  // apply borders
  m_opts.size.h += m_opts.borders[edge::TOP].size;
  m_opts.size.h += m_opts.borders[edge::BOTTOM].size;

  if (m_opts.origin == edge::BOTTOM) {
    m_opts.pos.y = m_opts.monitor->y + m_opts.monitor->h - m_opts.size.h - m_opts.offset.y;
  }

  if (m_opts.size.w <= 0 || m_opts.size.w > m_opts.monitor->w) {
    throw application_error("Resulting bar width is out of bounds");
  }
  if (m_opts.size.h <= 0 || m_opts.size.h > m_opts.monitor->h) {
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

  m_log.info("Bar geometry: %ix%i+%i+%i", m_opts.size.w, m_opts.size.h, m_opts.pos.x, m_opts.pos.y);
}

/**
 * Create monitor object
 */
void bar::setup_monitor() {
  m_log.trace("bar: Create monitor from matching X RandR output");

  auto strict = m_conf.get<bool>(m_conf.bar_section(), "monitor-strict", false);
  auto monitors = randr_util::get_monitors(m_connection, m_screen->root(), strict);

  if (monitors.empty()) {
    throw application_error("No monitors found");
  }

  auto name = m_conf.get<string>(m_conf.bar_section(), "monitor", "");

  if (name.empty()) {
    name = monitors[0]->name;
    m_log.warn("No monitor specified, using \"%s\"", name);
  }

  for (auto&& monitor : monitors) {
    if (monitor->match(name, strict)) {
      m_opts.monitor = move(monitor);
      break;
    }
  }

  if (!m_opts.monitor) {
    throw application_error("Monitor \"" + name + "\" not found or disconnected");
  }

  const auto& m = m_opts.monitor;
  m_log.trace("bar: Loaded monitor %s (%ix%i+%i+%i)", m->name, m->w, m->h, m->x, m->y);
}

/**
 * Move the bar window above defined sibling
 * in the X window stack
 */
void bar::restack_window() {
  string wm_restack;

  try {
    wm_restack = m_conf.get<string>(m_conf.bar_section(), "wm-restack");
  } catch (const key_error& err) {
    return;
  }

  auto restacked = false;

  if (wm_restack == "bspwm") {
    restacked = bspwm_util::restack_above_root(m_connection, m_opts.monitor, m_window);
#if ENABLE_I3
  } else if (wm_restack == "i3" && m_opts.override_redirect) {
    restacked = i3_util::restack_above_root(m_connection, m_opts.monitor, m_window);
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
  window win{m_connection, m_window};
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

  window win{m_connection, m_window};
  win.reconfigure_struts(w, h, m_opts.pos.x, m_opts.origin == edge::BOTTOM);
}

/**
 * Reconfigure window wm hint values
 */
void bar::reconfigure_wm_hints() {
  m_log.trace("bar: Set window WM_NAME");
  xcb_icccm_set_wm_name(m_connection, m_window, XCB_ATOM_STRING, 8, m_opts.wmname.size(), m_opts.wmname.c_str());
  xcb_icccm_set_wm_class(m_connection, m_window, 15, "polybar\0Polybar");

  m_log.trace("bar: Set window _NET_WM_WINDOW_TYPE");
  wm_util::set_windowtype(m_connection, m_window, {_NET_WM_WINDOW_TYPE_DOCK});

  m_log.trace("bar: Set window _NET_WM_STATE");
  wm_util::set_wmstate(m_connection, m_window, {_NET_WM_STATE_STICKY, _NET_WM_STATE_ABOVE});

  m_log.trace("bar: Set window _NET_WM_DESKTOP");
  wm_util::set_wmdesktop(m_connection, m_window, 0xFFFFFFFF);

  m_log.trace("bar: Set window _NET_WM_PID");
  wm_util::set_wmpid(m_connection, m_window, getpid());
}

/**
 * Broadcast current map state
 */
void bar::broadcast_visibility() {
  if (!g_signals::bar::visibility_change) {
    return m_log.trace("bar: no callback handler set for bar visibility change");
  }

  try {
    auto attr = m_connection.get_window_attributes(m_window);

    if (attr->map_state == XCB_MAP_STATE_UNVIEWABLE) {
      g_signals::bar::visibility_change(false);
    } else if (attr->map_state == XCB_MAP_STATE_UNMAPPED) {
      g_signals::bar::visibility_change(false);
    } else {
      g_signals::bar::visibility_change(true);
    }
  } catch (const exception& err) {
    return;
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
    }

    m_log.trace("Found matching input area");
    m_log.trace_x("action.command = %s", action.command);
    m_log.trace_x("action.button = %i", evt->detail);
    m_log.trace_x("action.start_x = %i", action.start_x);
    m_log.trace_x("action.end_x = %i", action.end_x);

    if (g_signals::bar::action_click) {
      g_signals::bar::action_click(action.command);
    }

    return;
  }

  for (auto&& action : m_opts.actions) {
    if (action.button == button && !action.command.empty()) {
      m_log.trace("Triggering fallback click handler: %s", action.command);

      if (g_signals::bar::action_click) {
        g_signals::bar::action_click(action.command);
      }

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
  if (evt->window == m_window && evt->count == 0) {
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

  if (evt->window == m_window && evt->atom == WM_STATE) {
    broadcast_visibility();
  }
}

POLYBAR_NS_END
