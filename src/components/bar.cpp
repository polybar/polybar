#include <xcb/xcb_icccm.h>

#include "components/bar.hpp"
#include "components/parser.hpp"
#include "components/renderer.hpp"
#include "components/signals.hpp"
#include "utils/bspwm.hpp"
#include "utils/color.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"
#include "x11/fonts.hpp"
#include "x11/graphics.hpp"
#include "x11/tray.hpp"
#include "x11/wm.hpp"

#if ENABLE_I3
#include "utils/i3.hpp"
#endif

POLYBAR_NS

/**
 * Configure injection module
 */
di::injector<unique_ptr<bar>> configure_bar() {
  // clang-format off
  return di::make_injector(
      configure_connection(),
      configure_config(),
      configure_logger(),
      configure_tray_manager());
  // clang-format on
}

/**
 * Construct bar instance
 */
bar::bar(connection& conn, const config& config, const logger& logger, unique_ptr<tray_manager> tray_manager)
    : m_connection(conn), m_conf(config), m_log(logger), m_tray(forward<decltype(tray_manager)>(tray_manager)) {}

/**
 * Cleanup signal handlers and destroy the bar window
 */
bar::~bar() {
  std::lock_guard<std::mutex> guard(m_mutex);

  // Disconnect signal handlers {{{
  g_signals::parser::alignment_change = nullptr;
  g_signals::parser::attribute_set = nullptr;
  g_signals::parser::attribute_unset = nullptr;
  g_signals::parser::action_block_open = nullptr;
  g_signals::parser::action_block_close = nullptr;
  g_signals::parser::color_change = nullptr;
  g_signals::parser::font_change = nullptr;
  g_signals::parser::pixel_offset = nullptr;
  g_signals::parser::ascii_text_write = nullptr;
  g_signals::parser::unicode_text_write = nullptr;
  g_signals::parser::string_write = nullptr;
  g_signals::tray::report_slotcount = nullptr;  // }}}

  if (m_tray) {
    m_tray.reset();
  }

  if (m_sinkattached) {
    m_connection.detach_sink(this, 1);
  }
}

/**
 * Create required components
 *
 * This is done outside the constructor due to boost::di noexcept
 */
void bar::bootstrap(bool nodraw) {
  auto bs = m_conf.bar_section();

  m_screen = m_connection.screen();

  auto geom = m_connection.get_geometry(m_screen->root);
  m_screensize.w = geom->width;
  m_screensize.h = geom->height;

  m_opts.locale = m_conf.get<string>(bs, "locale", "");
  m_opts.separator = string_util::trim(m_conf.get<string>(bs, "separator", ""), '"');

  create_monitor();

  // Set bar colors {{{

  m_opts.background = color::parse(m_conf.get<string>(bs, "background", color_util::hex<uint16_t>(m_opts.background)));
  m_opts.foreground = color::parse(m_conf.get<string>(bs, "foreground", color_util::hex<uint16_t>(m_opts.foreground)));

  auto linecolor = color::parse(m_conf.get<string>(bs, "linecolor", "#f00"));
  auto lineheight = m_conf.get<int>(bs, "lineheight", 0);

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

  // }}}
  // Set border values {{{

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

  // }}}
  // Set size and position {{{

  if (m_conf.get<bool>(bs, "bottom", false))
    m_opts.origin = edge::BOTTOM;
  else
    m_opts.origin = edge::TOP;

  GET_CONFIG_VALUE(bs, m_opts.force_docking, "dock");
  GET_CONFIG_VALUE(bs, m_opts.spacing, "spacing");
  GET_CONFIG_VALUE(bs, m_opts.padding.left, "padding-left");
  GET_CONFIG_VALUE(bs, m_opts.padding.right, "padding-right");
  GET_CONFIG_VALUE(bs, m_opts.module_margin.left, "module-margin-left");
  GET_CONFIG_VALUE(bs, m_opts.module_margin.right, "module-margin-right");

  m_opts.strut.top = m_conf.get<int>("global/wm", "margin-top", 0);
  m_opts.strut.bottom = m_conf.get<int>("global/wm", "margin-bottom", 0);

  // }}}
  // Set the WM_NAME value {{{
  // Required early for --print-wmname

  m_opts.wmname = m_conf.get<string>(bs, "wm-name", "polybar-" + bs.substr(4) + "_" + m_opts.monitor->name);
  m_opts.wmname = string_util::replace(m_opts.wmname, " ", "-");

  // }}}
  // Check nodraw flag {{{

  if (nodraw) {
    m_log.trace("bar: Abort bootstrap routine (reason: nodraw)");
    m_tray.reset();
    return;
  }

  // }}}
  // Connect signal handlers and attach sink {{{

  m_log.trace("bar: Attach parser callbacks");

  // clang-format off
  g_signals::parser::alignment_change = [this](const alignment align) {
    m_renderer->set_alignment(align);
  };
  g_signals::parser::attribute_set = [this](const attribute attr) {
    m_renderer->set_attribute(attr, true);
  };
  g_signals::parser::attribute_unset = [this](const attribute attr) {
    m_renderer->set_attribute(attr, false);
  };
  g_signals::parser::action_block_open = [this](const mousebtn btn, string cmd) {
    m_renderer->begin_action(btn, cmd);
  };
  g_signals::parser::action_block_close = [this](const mousebtn btn) {
    m_renderer->end_action(btn);
  };
  g_signals::parser::color_change= [this](const gc gcontext, const uint32_t color) {
    m_renderer->set_foreground(gcontext, color);
  };
  g_signals::parser::font_change = [this](const int8_t font) {
    m_renderer->set_fontindex(font);
  };
  g_signals::parser::pixel_offset = [this](const int16_t px) {
    m_renderer->shift_content(px);
  };
  g_signals::parser::ascii_text_write = [this](const uint16_t c) {
    m_renderer->draw_character(c);
  };
  g_signals::parser::unicode_text_write = [this](const uint16_t c) {
    m_renderer->draw_character(c);
  };
  g_signals::parser::string_write = [this](const char* text, const size_t len) {
    m_renderer->draw_textstring(text, len);
  };
  // clang-format on

  m_log.trace("bar: Attaching sink to registry");
  m_connection.attach_sink(this, 1);
  m_sinkattached = true;

  // }}}

  configure_geom();

  m_renderer = configure_renderer(m_opts, m_conf.get_list<string>(bs, "font", {})).create<unique_ptr<renderer>>();
  m_window = m_renderer->window();

  restack_window();
  set_wmhints();
  map_window();

  m_connection.flush();

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

  if (tray_position == "left")
    settings.align = alignment::LEFT;
  else if (tray_position == "right")
    settings.align = alignment::RIGHT;
  else if (tray_position == "center")
    settings.align = alignment::CENTER;
  else
    settings.align = alignment::NONE;

  if (settings.align == alignment::NONE) {
    m_log.warn("Disabling tray manager (reason: disabled in config)");
    m_tray.reset();
    return;
  }

  m_trayalign = settings.align;

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

  if (settings.align == alignment::RIGHT) {
    settings.orig_x = m_opts.pos.x + m_opts.size.w - m_opts.borders.at(edge::RIGHT).size;
  } else if (settings.align == alignment::LEFT) {
    settings.orig_x = m_opts.pos.x + m_opts.borders.at(edge::LEFT).size;
  } else if (settings.align == alignment::CENTER) {
    settings.orig_x = get_centerx() - (settings.width / 2);
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

  if (offset_x != 0 && offset_x_def.find("%") != string::npos) {
    offset_x = math_util::percentage_to_value<int>(offset_x, m_opts.monitor->w);
    offset_x -= settings.width / 2;
  }

  if (offset_y != 0 && offset_y_def.find("%") != string::npos) {
    offset_y = math_util::percentage_to_value<int>(offset_y, m_opts.monitor->h);
    offset_y -= settings.width / 2;
  }

  settings.orig_x += offset_x;
  settings.orig_y += offset_y;

  // Add tray update callback unless explicitly disabled
  if (!m_conf.get<bool>(bs, "tray-detached", false)) {
    g_signals::tray::report_slotcount = bind(&bar::on_tray_report, this, placeholders::_1);
  }

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
void bar::parse(string data, bool force) {
  if (!m_mutex.try_lock()) {
    return;
  }

  std::lock_guard<std::mutex> guard(m_mutex, std::adopt_lock);

  if (data == m_lastinput && !force)
    return;

  m_lastinput = data;

  if (m_trayclients) {
    if (m_tray && m_trayalign == alignment::LEFT)
      m_renderer->reserve_space(edge::LEFT, m_tray->settings().configured_w);
    else if (m_tray && m_trayalign == alignment::RIGHT)
      m_renderer->reserve_space(edge::RIGHT, m_tray->settings().configured_w);
  }

  m_renderer->begin();

  try {
    parser parser(m_opts);
    parser(data);
  } catch (const unrecognized_token& err) {
    m_log.err("Unrecognized syntax token '%s'", err.what());
  }

  m_renderer->end();
}

/**
 * Refresh the bar window by clearing and redrawing the pixmaps
 */
void bar::refresh_window() {
  m_log.info("Refresh bar window");
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
  if ((m_opts.size.w = atoi(w.c_str())) && w.find("%") != string::npos) {
    m_opts.size.w = math_util::percentage_to_value<int>(m_opts.size.w, m_opts.monitor->w);
  }

  // look for user-defined  height
  if ((m_opts.size.h = atoi(h.c_str())) && h.find("%") != string::npos) {
    m_opts.size.h = math_util::percentage_to_value<int>(m_opts.size.h, m_opts.monitor->h);
  }

  // look for user-defined offset-x
  if ((m_opts.offset.x = atoi(offsetx.c_str())) != 0 && offsetx.find("%") != string::npos) {
    m_opts.offset.x = math_util::percentage_to_value<int>(m_opts.offset.x, m_opts.monitor->w);
  }

  // look for user-defined offset-y
  if ((m_opts.offset.y = atoi(offsety.c_str())) != 0 && offsety.find("%") != string::npos) {
    m_opts.offset.y = math_util::percentage_to_value<int>(m_opts.offset.y, m_opts.monitor->h);
  }

  // apply offsets
  m_opts.pos.x = m_opts.offset.x + m_opts.monitor->x;
  m_opts.pos.y = m_opts.offset.y + m_opts.monitor->y;

  // apply borders
  m_opts.size.h += m_opts.borders[edge::TOP].size;
  m_opts.size.h += m_opts.borders[edge::BOTTOM].size;

  if (m_opts.origin == edge::BOTTOM)
    m_opts.pos.y = m_opts.monitor->y + m_opts.monitor->h - m_opts.size.h - m_opts.offset.y;

  if (m_opts.size.w <= 0 || m_opts.size.w > m_opts.monitor->w)
    throw application_error("Resulting bar width is out of bounds");
  if (m_opts.size.h <= 0 || m_opts.size.h > m_opts.monitor->h)
    throw application_error("Resulting bar height is out of bounds");

  m_opts.size.w = math_util::cap<int>(m_opts.size.w, 0, m_opts.monitor->w);
  m_opts.size.h = math_util::cap<int>(m_opts.size.h, 0, m_opts.monitor->h);

  m_opts.center.y = (m_opts.size.h + m_opts.borders[edge::TOP].size - m_opts.borders[edge::BOTTOM].size) / 2;

  m_log.info("Bar geometry %ix%i+%i+%i", m_opts.size.w, m_opts.size.h, m_opts.pos.x, m_opts.pos.y);
}

/**
 * Create monitor object
 */
void bar::create_monitor() {
  m_log.trace("bar: Create monitor from matching X RandR output");

  auto strict = m_conf.get<bool>(m_conf.bar_section(), "monitor-strict", false);
  auto monitors = randr_util::get_monitors(m_connection, m_screen->root, strict);

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
  } else if (wm_restack == "i3" && m_opts.force_docking) {
    restacked = i3_util::restack_above_root(m_connection, m_opts.monitor, m_window);
  } else if (wm_restack == "i3" && !m_opts.force_docking) {
    m_log.warn("Ignoring restack of i3 window (not needed when dock = false)");
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
 * Map window and reconfigure its position
 */
void bar::map_window() {
  auto geom = m_connection.get_geometry(m_screen->root);
  auto w = m_opts.size.w + m_opts.offset.x;
  auto h = m_opts.size.h + m_opts.offset.y;
  auto x = m_opts.pos.x;
  auto y = m_opts.pos.y;

  if (m_opts.origin == edge::BOTTOM) {
    h += m_opts.strut.top;
  } else {
    h += m_opts.strut.bottom;
  }

  if (m_opts.origin == edge::BOTTOM && m_opts.monitor->y + m_opts.monitor->h < m_screensize.h) {
    h += m_screensize.h - (m_opts.monitor->y + m_opts.monitor->h);
  } else if (m_opts.origin != edge::BOTTOM) {
    h += m_opts.monitor->y;
  }

  window win{m_connection, m_window};
  win.map_checked();
  win.reconfigure_struts(w, h, x, m_opts.origin == edge::BOTTOM);
  win.reconfigure_pos(x, y);
}

/**
 * Set window atom values
 */
void bar::set_wmhints() {
  m_log.trace("bar: Set WM_NAME");
  xcb_icccm_set_wm_name(m_connection, m_window, XCB_ATOM_STRING, 8, m_opts.wmname.size(), m_opts.wmname.c_str());
  xcb_icccm_set_wm_class(m_connection, m_window, 15, "polybar\0Polybar");

  m_log.trace("bar: Set WM_NORMAL_HINTS");
  xcb_size_hints_t hints;
  xcb_icccm_size_hints_set_position(&hints, true, m_opts.pos.x, m_opts.pos.y);
  xcb_icccm_size_hints_set_size(&hints, true, m_opts.size.w, m_opts.size.h);
  xcb_icccm_set_wm_normal_hints(m_connection, m_window, &hints);

  m_log.trace("bar: Set _NET_WM_WINDOW_TYPE");
  wm_util::set_windowtype(m_connection, m_window, {_NET_WM_WINDOW_TYPE_DOCK});

  m_log.trace("bar: Set _NET_WM_STATE");
  wm_util::set_wmstate(m_connection, m_window, {_NET_WM_STATE_STICKY, _NET_WM_STATE_ABOVE});

  m_log.trace("bar: Set _NET_WM_DESKTOP");
  wm_util::set_wmdesktop(m_connection, m_window, 0xFFFFFFFF);

  m_log.trace("bar: Set _NET_WM_PID");
  wm_util::set_wmpid(m_connection, m_window, getpid());
}

/**
 * Get the horizontal center pos
 */
int bar::get_centerx() {
  int x = m_opts.pos.x;
  x += m_opts.size.w;
  x -= m_opts.borders[edge::RIGHT].size;
  x += m_opts.borders[edge::LEFT].size;
  x /= 2;
  return x;
}

/**
 * Get the inner width of the bar
 */
int bar::get_innerwidth() {
  auto w = m_opts.size.w;
  w -= m_opts.borders[edge::RIGHT].size;
  w -= m_opts.borders[edge::LEFT].size;
  return w;
}

/**
 * Event handler for XCB_BUTTON_PRESS events
 *
 * Used to map mouse clicks to bar actions
 */
void bar::handle(const evt::button_press& evt) {
  std::lock_guard<std::mutex> guard(m_mutex, std::adopt_lock);

  m_log.trace_x("bar: Received button press: %i at pos(%i, %i)", evt->detail, evt->event_x, evt->event_y);

  mousebtn button = static_cast<mousebtn>(evt->detail);

  for (auto&& action : m_renderer->get_actions()) {
    if (action.active) {
      m_log.trace_x("bar: Ignoring action: unclosed)");
      continue;
    } else if (action.button != button) {
      m_log.trace_x("bar: Ignoring action: button mismatch");
      continue;
    } else if (action.start_x > evt->event_x) {
      m_log.trace_x("bar: Ignoring action: start_x(%i) > event_x(%i)", action.start_x, evt->event_x);
      continue;
    } else if (action.end_x < evt->event_x) {
      m_log.trace_x("bar: Ignoring action: end_x(%i) < event_x(%i)", action.end_x, evt->event_x);
      continue;
    }

    m_log.trace("Found matching input area");
    m_log.trace_x("action.command = %s", action.command);
    m_log.trace_x("action.button = %i", static_cast<int>(action.button));
    m_log.trace_x("action.start_x = %i", action.start_x);
    m_log.trace_x("action.end_x = %i", action.end_x);

    if (g_signals::bar::action_click)
      g_signals::bar::action_click(action.command);
    else
      m_log.warn("No signal handler's connected to 'action_click'");

    return;
  }

  m_log.warn("No matching input area found");
}

/**
 * Event handler for XCB_EXPOSE events
 *
 * Used to redraw the bar
 */
void bar::handle(const evt::expose& evt) {
  if (evt->window == m_window) {
    m_log.trace("bar: Received expose event");
    m_renderer->redraw();
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
  (void)evt;
#if DEBUG
  string atom_name = m_connection.get_atom_name(evt->atom).name();
  m_log.trace("bar: property_notify(%s)", atom_name);
#endif

  if (evt->window == m_window && evt->atom == WM_STATE) {
    if (!g_signals::bar::visibility_change) {
      return;
    }

    try {
      auto attr = m_connection.get_window_attributes(m_window);
      if (attr->map_state == XCB_MAP_STATE_VIEWABLE)
        g_signals::bar::visibility_change(true);
      else if (attr->map_state == XCB_MAP_STATE_UNVIEWABLE)
        g_signals::bar::visibility_change(false);
      else if (attr->map_state == XCB_MAP_STATE_UNMAPPED)
        g_signals::bar::visibility_change(false);
      else
        g_signals::bar::visibility_change(true);
    } catch (const exception& err) {
      m_log.warn("Failed to emit bar window's visibility change event");
    }
  } else if (evt->atom == _XROOTMAP_ID) {
    refresh_window();
  } else if (evt->atom == _XSETROOT_ID) {
    refresh_window();
  } else if (evt->atom == ESETROOT_PMAP_ID) {
    refresh_window();
  }
}

/**
 * Proess systray report
 */
void bar::on_tray_report(uint16_t slots) {
  if (m_trayclients == slots) {
    return;
  }

  m_log.trace("bar: tray_report(%lu)", slots);
  m_trayclients = slots;

  if (!m_lastinput.empty()) {
    parse(m_lastinput, true);
  }
}

POLYBAR_NS_END
