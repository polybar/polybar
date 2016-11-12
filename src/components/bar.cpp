#include <xcb/xcb_icccm.h>

#include "components/bar.hpp"
#include "utils/bspwm.hpp"
#include "utils/color.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"
#include "x11/draw.hpp"
#include "x11/randr.hpp"
#include "x11/wm.hpp"
#include "x11/xlib.hpp"
#include "x11/xutils.hpp"

#if ENABLE_I3
#include "utils/i3.hpp"
#endif

LEMONBUDDY_NS

/**
 * Cleanup signal handlers and destroy the bar window
 */
bar::~bar() {  // {{{
  std::lock_guard<threading_util::spin_lock> lck(m_lock);

  // Disconnect signal handlers {{{
  g_signals::parser::alignment_change = nullptr;
  g_signals::parser::attribute_set = nullptr;
  g_signals::parser::attribute_unset = nullptr;
  g_signals::parser::attribute_toggle = nullptr;
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

  m_window.destroy();
}  // }}}

/**
 * Create required components
 *
 * This is done outside the constructor due to boost::di noexcept
 */
void bar::bootstrap(bool nodraw) {  // {{{
  auto bs = m_conf.bar_section();

  m_screen = m_connection.screen();

  // limit the amount of allowed input events to 1 per 60ms
  m_throttler = throttle_util::make_throttler(1, 60ms);

  m_opts.separator = string_util::trim(m_conf.get<string>(bs, "separator", ""), '"');
  m_opts.locale = m_conf.get<string>(bs, "locale", "");

  create_monitor();

  // Set bar colors {{{

  m_opts.background = color::parse(m_conf.get<string>(bs, "background", m_opts.background.source()));
  m_opts.foreground = color::parse(m_conf.get<string>(bs, "foreground", m_opts.foreground.source()));
  m_opts.linecolor = color::parse(m_conf.get<string>(bs, "linecolor", m_opts.linecolor.source()));

  // }}}
  // Set border values {{{

  auto bsize = m_conf.get<int>(bs, "border-size", 0);
  auto bcolor = m_conf.get<string>(bs, "border-color", g_colorempty.source());

  m_borders.emplace(border::TOP, border_settings{});
  m_borders[border::TOP].size = m_conf.get<int>(bs, "border-top", bsize);
  m_borders[border::TOP].color = color::parse(m_conf.get<string>(bs, "border-top-color", bcolor));

  m_borders.emplace(border::BOTTOM, border_settings{});
  m_borders[border::BOTTOM].size = m_conf.get<int>(bs, "border-bottom", bsize);
  m_borders[border::BOTTOM].color = color::parse(m_conf.get<string>(bs, "border-bottom-color", bcolor));

  m_borders.emplace(border::LEFT, border_settings{});
  m_borders[border::LEFT].size = m_conf.get<int>(bs, "border-left", bsize);
  m_borders[border::LEFT].color = color::parse(m_conf.get<string>(bs, "border-left-color", bcolor));

  m_borders.emplace(border::RIGHT, border_settings{});
  m_borders[border::RIGHT].size = m_conf.get<int>(bs, "border-right", bsize);
  m_borders[border::RIGHT].color = color::parse(m_conf.get<string>(bs, "border-right-color", bcolor));

  // }}}
  // Set size and position {{{

  GET_CONFIG_VALUE(bs, m_opts.dock, "dock");
  GET_CONFIG_VALUE(bs, m_opts.bottom, "bottom");
  GET_CONFIG_VALUE(bs, m_opts.spacing, "spacing");
  GET_CONFIG_VALUE(bs, m_opts.lineheight, "lineheight");
  GET_CONFIG_VALUE(bs, m_opts.padding_left, "padding-left");
  GET_CONFIG_VALUE(bs, m_opts.padding_right, "padding-right");
  GET_CONFIG_VALUE(bs, m_opts.module_margin_left, "module-margin-left");
  GET_CONFIG_VALUE(bs, m_opts.module_margin_right, "module-margin-right");

  // }}}
  // Set the WM_NAME value {{{
  // Required early for --print-wmname

  m_opts.wmname = "lemonbuddy-" + bs.substr(4) + "_" + m_opts.monitor->name;
  m_opts.wmname = m_conf.get<string>(bs, "wm-name", m_opts.wmname);
  m_opts.wmname = string_util::replace(m_opts.wmname, " ", "-");

  // }}}
  // Check nodraw flag {{{

  if (nodraw) {
    m_log.trace("bar: Abort bootstrap routine (reason: nodraw)");
    m_tray.reset();
    return;
  }

  // }}}
  // Setup graphic components and create window {{{

  m_log.trace("bar: Get true color visual");
  m_visual = m_connection.visual_type(m_screen, 32).get();

  m_log.trace("bar: Create colormap");
  m_colormap = colormap{m_connection, m_connection.generate_id()};
  m_connection.create_colormap(XCB_COLORMAP_ALLOC_NONE, m_colormap, m_screen->root, m_visual->visual_id);

  configure_geom();
  create_window();
  create_pixmap();
  create_gcontexts();
  create_rootpixmap();
  restack_window();
  set_wmhints();

  // m_log.trace("bar: Listen to X RandR events");
  // m_connection.select_input(m_window, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);

  m_connection.map_window(m_window);

  // }}}
  // Connect signal handlers {{{

  m_log.trace("bar: Attach parser callbacks");

  // clang-format off
  g_signals::parser::alignment_change = bind(&bar::on_alignment_change, this, placeholders::_1);
  g_signals::parser::attribute_set = bind(&bar::on_attribute_set, this, placeholders::_1);
  g_signals::parser::attribute_unset = bind(&bar::on_attribute_unset, this, placeholders::_1);
  g_signals::parser::attribute_toggle = bind(&bar::on_attribute_toggle, this, placeholders::_1);
  g_signals::parser::action_block_open = bind(&bar::on_action_block_open, this, placeholders::_1, placeholders::_2);
  g_signals::parser::action_block_close = bind(&bar::on_action_block_close, this, placeholders::_1);
  g_signals::parser::color_change = bind(&bar::on_color_change, this, placeholders::_1, placeholders::_2);
  g_signals::parser::font_change = bind(&bar::on_font_change, this, placeholders::_1);
  g_signals::parser::pixel_offset = bind(&bar::on_pixel_offset, this, placeholders::_1);
  g_signals::parser::ascii_text_write = bind(&bar::draw_character, this, placeholders::_1);
  g_signals::parser::unicode_text_write = bind(&bar::draw_character, this, placeholders::_1);
  g_signals::parser::string_write = bind(&bar::draw_textstring, this, placeholders::_1, placeholders::_2);
  // clang-format on

  // }}}

  m_log.trace("bar: Attaching sink to registry");
  m_connection.attach_sink(this, 1);
  m_sinkattached = true;

  load_fonts();

  m_connection.flush();
}  // }}}

/**
 * Setup tray manager
 */
void bar::bootstrap_tray() {  // {{{
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

  m_traypos = settings.align;

  settings.height = m_opts.height;
  settings.height -= m_borders.at(border::BOTTOM).size;
  settings.height -= m_borders.at(border::TOP).size;
  settings.height_fill = settings.height;

  if (settings.height % 2 != 0) {
    settings.height--;
  }

  auto maxsize = m_conf.get<int>(bs, "tray-maxsize", 16);
  if (settings.height > maxsize) {
    settings.spacing += (settings.height - maxsize) / 2;
    settings.height = maxsize;
  }

  settings.width_max = m_opts.width;
  settings.width = settings.height;
  settings.orig_y = m_opts.y + m_borders.at(border::TOP).size;

  // Apply user-defined scaling
  auto scale = m_conf.get<float>(bs, "tray-scale", 1.0);
  settings.width *= scale;
  settings.height_fill *= scale;

  if (settings.align == alignment::RIGHT) {
    settings.orig_x = m_opts.x + m_opts.width - m_borders.at(border::RIGHT).size;
  } else if (settings.align == alignment::LEFT) {
    settings.orig_x = m_opts.x + m_borders.at(border::LEFT).size;
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
    offset_x = math_util::percentage_to_value(offset_x, m_opts.monitor->w);
    offset_x -= settings.width / 2;
  }

  if (offset_y != 0 && offset_y_def.find("%") != string::npos) {
    offset_y = math_util::percentage_to_value(offset_y, m_opts.monitor->h);
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
}  // }}}

/**
 * Activate tray manager
 */
void bar::activate_tray() {  // {{{
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
}  // }}}

/**
 * Get the bar settings container
 */
const bar_settings bar::settings() const {  // {{{
  return m_opts;
}  // }}}

/**
 * Parse input string and redraw the bar window
 *
 * @param data Input string
 * @param force Unless true, do not parse unchanged data
 */
void bar::parse(string data, bool force) {  // {{{
  std::lock_guard<threading_util::spin_lock> lck(m_lock);
  {
    if (data == m_prevdata && !force)
      return;

    m_prevdata = data;

    // TODO: move to fontmanager
    m_xftdraw = XftDrawCreate(xlib::get_display(), m_pixmap, xlib::get_visual(), m_colormap);

    m_opts.align = alignment::LEFT;
    m_xpos = m_borders[border::LEFT].size;
    m_attributes = 0;

#if DEBUG and DRAW_CLICKABLE_AREA_HINTS
    for (auto&& action : m_actions) {
      m_connection.destroy_window(action.clickable_area);
    }
#endif

    m_actions.clear();

    draw_background();

    if (m_traypos == alignment::LEFT && m_trayclients > 0) {
      auto& tray = m_tray->settings();
      m_xpos += ((tray.width + tray.spacing) * m_trayclients) + tray.spacing;
    }

    try {
      parser parser(m_opts);
      parser(data);
    } catch (const unrecognized_token& err) {
      m_log.err("Unrecognized syntax token '%s'", err.what());
    }

    if (m_traypos == alignment::RIGHT && m_trayclients > 0) {
      auto& tray = m_tray->settings();
      draw_shift(m_xpos, ((tray.width + tray.spacing) * m_trayclients) + tray.spacing);
    }

    draw_border(border::ALL);

    XftDrawDestroy(m_xftdraw);

    flush();
  }
}  // }}}

/**
 * Copy the contents of the pixmap's onto the bar window
 */
void bar::flush() {  // {{{
  m_connection.copy_area(m_pixmap, m_window, m_gcontexts.at(gc::FG), 0, 0, 0, 0, m_opts.width, m_opts.height);
  m_connection.flush();

  if (g_signals::bar::redraw) {
    g_signals::bar::redraw();
  }

#if DEBUG and DRAW_CLICKABLE_AREA_HINTS
  map<alignment, int> hint_num{{
      {alignment::LEFT, 0}, {alignment::CENTER, 0}, {alignment::RIGHT, 0},
  }};
#endif

  for (auto&& action : m_actions) {
    if (action.active) {
      m_log.warn("Action block not closed");
      m_log.warn("action.command = %s", action.command);
    } else {
      m_log.trace_x("bar: Action details (button = %i, start_x = %i, end_x = %i, command = '%s')",
          static_cast<int>(action.button), action.start_x, action.end_x, action.command);
#if DEBUG and DRAW_CLICKABLE_AREA_HINTS
      m_log.info("Drawing clickable area hints");

      hint_num[action.align]++;

      auto x = action.start_x;
      auto y = m_opts.y + hint_num[action.align]++ * DRAW_CLICKABLE_AREA_HINTS_OFFSET_Y;
      auto w = action.end_x - action.start_x - 2;
      auto h = m_opts.height - 2;

      const uint32_t mask = XCB_CW_BORDER_PIXEL | XCB_CW_OVERRIDE_REDIRECT;
      const uint32_t border_color = hint_num[action.align] % 2 ? 0xff0000 : 0x00ff00;
      const uint32_t values[2]{border_color, true};

      action.clickable_area = window{m_connection, m_connection.generate_id()};
      m_connection.create_window_checked(m_screen->root_depth, action.clickable_area, m_screen->root, x, y, w, h, 1,
          XCB_WINDOW_CLASS_INPUT_OUTPUT, m_screen->root_visual, mask, values);
      m_connection.map_window_checked(action.clickable_area);
#endif
    }
  }
}  // }}}

/**
 * Refresh the bar window by clearing and redrawing the pixmaps
 */
void bar::refresh_window() {  // {{{
  m_log.info("Refresh bar window");
}  // }}}

/**
 * Load user-defined fonts
 */
void bar::load_fonts() {  // {{{
  auto fonts_loaded = false;
  auto fontindex = 0;
  auto fonts = m_conf.get_list<string>("font");

  for (auto f : fonts) {
    fontindex++;
    vector<string> fd = string_util::split(f, ';');
    string pattern{fd[0]};
    int offset{0};

    if (fd.size() > 1)
      offset = std::stoi(fd[1], 0, 10);

    if (m_fontmanager->load(pattern, fontindex, offset))
      fonts_loaded = true;
    else
      m_log.warn("Unable to load font '%s'", fd[0]);
  }

  if (!fonts_loaded) {
    m_log.warn("Loading fallback font");

    if (!m_fontmanager->load("fixed"))
      throw application_error("Unable to load fonts");
  }

  m_fontmanager->allocate_color(m_opts.foreground, true);
}  // }}}

/**
 * Configure geometry values
 */
void bar::configure_geom() {  // {{{
  m_log.trace("bar: Configure window geometry");

  auto w = m_conf.get<string>(m_conf.bar_section(), "width", "100%");
  auto h = m_conf.get<string>(m_conf.bar_section(), "height", "24");

  auto offsetx = m_conf.get<string>(m_conf.bar_section(), "offset-x", "");
  auto offsety = m_conf.get<string>(m_conf.bar_section(), "offset-y", "");

  // look for user-defined width
  if ((m_opts.width = atoi(w.c_str())) && w.find("%") != string::npos) {
    m_opts.width = math_util::percentage_to_value<int>(m_opts.width, m_opts.monitor->w);
  }

  // look for user-defined  height
  if ((m_opts.height = atoi(h.c_str())) && h.find("%") != string::npos) {
    m_opts.height = math_util::percentage_to_value<int>(m_opts.height, m_opts.monitor->h);
  }

  // look for user-defined offset-x
  if ((m_opts.offset_x = atoi(offsetx.c_str())) != 0 && offsetx.find("%") != string::npos) {
    m_opts.offset_x = math_util::percentage_to_value<int>(m_opts.offset_x, m_opts.monitor->w);
  }

  // look for user-defined offset-y
  if ((m_opts.offset_y = atoi(offsety.c_str())) != 0 && offsety.find("%") != string::npos) {
    m_opts.offset_y = math_util::percentage_to_value<int>(m_opts.offset_y, m_opts.monitor->h);
  }

  // apply offsets
  m_opts.x = m_opts.offset_x + m_opts.monitor->x;
  m_opts.y = m_opts.offset_y + m_opts.monitor->y;

  // apply borders
  m_opts.height += m_borders[border::TOP].size;
  m_opts.height += m_borders[border::BOTTOM].size;

  if (m_opts.bottom)
    m_opts.y = m_opts.monitor->y + m_opts.monitor->h - m_opts.height - m_opts.offset_y;

  if (m_opts.width <= 0 || m_opts.width > m_opts.monitor->w)
    throw application_error("Resulting bar width is out of bounds");
  if (m_opts.height <= 0 || m_opts.height > m_opts.monitor->h)
    throw application_error("Resulting bar height is out of bounds");

  m_opts.width = math_util::cap<int>(m_opts.width, 0, m_opts.monitor->w);
  m_opts.height = math_util::cap<int>(m_opts.height, 0, m_opts.monitor->h);

  m_opts.vertical_mid = (m_opts.height + m_borders[border::TOP].size - m_borders[border::BOTTOM].size) / 2;

  m_log.info("Bar geometry %ix%i+%i+%i", m_opts.width, m_opts.height, m_opts.x, m_opts.y);
}  // }}}

/**
 * Create monitor object
 */
void bar::create_monitor() {  // {{{
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
}  // }}}

/**
 * Create window object
 */
void bar::create_window() {  // {{{
  m_log.trace("bar: Create window %s", m_connection.id(m_window));

  uint32_t mask = 0;
  xcb_params_cw_t params;

  // clang-format off
  XCB_AUX_ADD_PARAM(&mask, &params, back_pixel, 0);
  XCB_AUX_ADD_PARAM(&mask, &params, border_pixel, 0);
  XCB_AUX_ADD_PARAM(&mask, &params, backing_store, XCB_BACKING_STORE_WHEN_MAPPED);
  XCB_AUX_ADD_PARAM(&mask, &params, colormap, m_colormap);
  XCB_AUX_ADD_PARAM(&mask, &params, override_redirect, m_opts.dock);
  XCB_AUX_ADD_PARAM(&mask, &params, event_mask, XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS);
  // clang-format on

  uint32_t values[16];
  xutils::pack_values(mask, &params, values);

  m_connection.create_window_checked(32, m_window, m_screen->root, m_opts.x, m_opts.y, m_opts.width, m_opts.height, 0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, m_visual->visual_id, mask, values);
}  // }}}

/**
 * Create window pixmap
 */
void bar::create_pixmap() {  // {{{
  m_log.trace("bar: Create pixmap (xid=%s)", m_connection.id(m_pixmap));
  m_connection.create_pixmap(32, m_pixmap, m_window, m_opts.width, m_opts.height);
}  // }}}

/**
 * Create window gcontexts
 */
void bar::create_gcontexts() {  // {{{
  // clang-format off
  vector<uint32_t> colors {
    m_opts.background,
    m_opts.foreground,
    m_opts.linecolor,
    m_opts.linecolor,
    m_borders[border::TOP].color,
    m_borders[border::BOTTOM].color,
    m_borders[border::LEFT].color,
    m_borders[border::RIGHT].color,
  };
  // clang-format on

  for (int i = 1; i <= 8; i++) {
    uint32_t mask = 0;
    uint32_t value_list[32];

    xcb_params_gc_t params;
    XCB_AUX_ADD_PARAM(&mask, &params, foreground, colors[i - 1]);
    XCB_AUX_ADD_PARAM(&mask, &params, graphics_exposures, 0);

    xutils::pack_values(mask, &params, value_list);
    m_gcontexts.emplace(gc(i), gcontext{m_connection, m_connection.generate_id()});

    m_log.trace("bar: Create gcontext (gc=%i, xid=%s)", i, m_connection.id(m_gcontexts.at(gc(i))));
    m_connection.create_gc(m_gcontexts.at(gc(i)), m_pixmap, mask, value_list);
  }
}  // }}}

/**
 * Create an X image from the root pixmap
 * used to render pseudo-transparent backgrounds
 */
void bar::create_rootpixmap() {  // {{{
#if 0
  m_log.trace("bar: Create root pixmap");
  graphics_util::get_root_pixmap(m_connection, &m_rootpixmap);
  graphics_util::simple_gc(m_connection, m_pixmap, &m_root_gc);

  if (!m_rootpixmap.pixmap || !m_pixmap || !m_root_gc) {
    m_log.warn("Failed to get root pixmap for bar window background");
  } else {
    m_log.trace("bar: rootpixmap=%x (%dx%d+%d+%d)", m_rootpixmap.pixmap, m_rootpixmap.width,
        m_rootpixmap.height, m_rootpixmap.x, m_rootpixmap.y);

    m_connection.copy_area(m_rootpixmap.pixmap, m_pixmap, m_root_gc, m_opts.x,
    m_opts.y, 0, 0,
        m_opts.width, m_opts.height);

    auto image_reply = m_connection.get_image(
        XCB_IMAGE_FORMAT_Z_PIXMAP, m_pixmap, 0, 0, m_opts.width, m_opts.height,
        XAllPlanes());

    vector<uint8_t> image_data;
    std::back_insert_iterator<decltype(image_data)> back_it(image_data);
    std::copy(image_reply.data().begin(), image_reply.data().end(), back_it);

    m_connection.put_image(XCB_IMAGE_FORMAT_Z_PIXMAP, m_pixmap, m_root_gc, m_opts.width,
        m_opts.height, 0, 0, 0, image_reply->depth, image_data.size(), image_data.data());

    m_connection.copy_area(m_rootpixmap.pixmap, m_pixmap, m_root_gc, m_opts.x,
    m_opts.y, 0, 0,
        m_opts.width, m_opts.height);

    uint32_t mask = 0;
    uint32_t values[16];
    xcb_params_cw_t params;
    XCB_AUX_ADD_PARAM(&mask, &params, back_pixmap, m_pixmap);
    xutils::pack_values(mask, &params, values);
    m_connection.change_window_attributes_checked(m_window, mask, values);

    m_connection.copy_area(
        m_pixmap, m_window, m_root_gc, m_opts.x, m_opts.y, 0, 0, m_opts.width,
        m_opts.height);
  }
#endif
}  // }}}

/**
 * Move the bar window above defined sibling
 * in the X window stack
 */
void bar::restack_window() {  // {{{
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
  } else if (wm_restack == "i3" && m_opts.dock) {
    restacked = i3_util::restack_above_root(m_connection, m_opts.monitor, m_window);
  } else if (wm_restack == "i3" && !m_opts.dock) {
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
}  // }}}

/**
 * Set window atom values
 */
void bar::set_wmhints() {  // {{{
  m_log.trace("bar: Set WM_NAME");
  xcb_icccm_set_wm_name(m_connection, m_window, XCB_ATOM_STRING, 8, m_opts.wmname.length(), m_opts.wmname.c_str());
  xcb_icccm_set_wm_class(m_connection, m_window, 21, "lemonbuddy\0Lemonbuddy");

  m_log.trace("bar: Set _NET_WM_WINDOW_TYPE");
  wm_util::set_windowtype(m_connection, m_window, {_NET_WM_WINDOW_TYPE_DOCK});

  m_log.trace("bar: Set _NET_WM_STATE");
  wm_util::set_wmstate(m_connection, m_window, {_NET_WM_STATE_STICKY, _NET_WM_STATE_ABOVE});

  m_log.trace("bar: Set _NET_WM_DESKTOP");
  wm_util::set_wmdesktop(m_connection, m_window, -1u);

  m_log.trace("bar: Set _NET_WM_PID");
  wm_util::set_wmpid(m_connection, m_window, getpid());

  m_log.trace("bar: Set _NET_WM_STRUT_PARTIAL");
  {
    uint32_t none{0};
    uint32_t value_list[12]{none};

    auto mt = m_conf.get<int>("global/wm", "margin-top", 0);
    auto mb = m_conf.get<int>("global/wm", "margin-bottom", 0);
    auto ml = m_conf.get<int>("global/wm", "margin-left", 0);
    auto mr = m_conf.get<int>("global/wm", "margin-right", 0);

    if (m_opts.bottom) {
      value_list[3] = m_opts.monitor->y + m_opts.height + mt;
      value_list[10] = m_opts.monitor->x + m_opts.x + ml;
      value_list[11] = m_opts.monitor->x + m_opts.x + m_opts.width + mr;
    } else {
      value_list[2] = m_opts.monitor->y + m_opts.height + mb;
      value_list[8] = m_opts.monitor->x + m_opts.x + ml;
      value_list[9] = m_opts.monitor->x + m_opts.x + m_opts.width + mr;
    }

    m_connection.change_property(
        XCB_PROP_MODE_REPLACE, m_window, _NET_WM_STRUT_PARTIAL, XCB_ATOM_CARDINAL, 32, 12, value_list);
  }
}  // }}}

/**
 * Get the horizontal center pos
 */
int bar::get_centerx() {  // {{{
  int x = m_opts.x;
  x += m_opts.width;
  x -= m_borders[border::RIGHT].size;
  x += m_borders[border::LEFT].size;
  x /= 2;
  return x;
}  // }}}

/**
 * Get the inner width of the bar
 */
int bar::get_innerwidth() {  // {{{
  auto w = m_opts.width;
  w -= m_borders[border::RIGHT].size;
  w -= m_borders[border::LEFT].size;
  return w;
}  // }}}

/**
 * Event handler for XCB_BUTTON_PRESS events
 *
 * Used to map mouse clicks to bar actions
 */
void bar::handle(const evt::button_press& evt) {  // {{{
  if (!m_throttler->passthrough(throttle_util::strategy::try_once_or_leave_yolo{})) {
    return;
  }

  std::lock_guard<threading_util::spin_lock> lck(m_lock);
  {
    m_log.trace_x("bar: Received button press event: %i at pos(%i, %i)", static_cast<int>(evt->detail), evt->event_x,
        evt->event_y);

    mousebtn button = static_cast<mousebtn>(evt->detail);

    for (auto&& action : m_actions) {
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
}  // }}}

/**
 * Event handler for XCB_EXPOSE events
 *
 * Used to redraw the bar
 */
void bar::handle(const evt::expose& evt) {  // {{{
  if (evt->window == m_window) {
    m_log.trace("bar: Received expose event");
    flush();
  }
}  // }}}

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
void bar::handle(const evt::property_notify& evt) {  // {{{
#ifdef DEBUG
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
}  // }}}

/**
 * Handle alignment update
 */
void bar::on_alignment_change(alignment align) {  // {{{
  if (align == m_opts.align)
    return;

  m_log.trace_x("bar: alignment_change(%i)", static_cast<int>(align));
  m_opts.align = align;

  if (align == alignment::LEFT) {
    m_xpos = m_borders[border::LEFT].size;
  } else if (align == alignment::RIGHT) {
    m_xpos = m_borders[border::RIGHT].size;
  } else {
    m_xpos = 0;
  }
}  // }}}

/**
 * Handle attribute on state
 */
void bar::on_attribute_set(attribute attr) {  // {{{
  int val{static_cast<int>(attr)};
  if ((m_attributes & val) != 0)
    return;
  m_log.trace_x("bar: attribute_set(%i)", val);
  m_attributes |= val;
}  // }}}

/**
 * Handle attribute off state
 */
void bar::on_attribute_unset(attribute attr) {  // {{{
  int val{static_cast<int>(attr)};
  if ((m_attributes & val) == 0)
    return;
  m_log.trace_x("bar: attribute_unset(%i)", val);
  m_attributes ^= val;
}  // }}}

/**
 * Handle attribute toggle state
 */
void bar::on_attribute_toggle(attribute attr) {  // {{{
  int val{static_cast<int>(attr)};
  m_log.trace_x("bar: attribute_toggle(%i)", val);
  m_attributes ^= val;
}  // }}}

/**
 * Handle action block start
 */
void bar::on_action_block_open(mousebtn btn, string cmd) {  // {{{
  if (btn == mousebtn::NONE)
    btn = mousebtn::LEFT;
  m_log.trace_x("bar: action_block_open(%i, %s)", static_cast<int>(btn), cmd);
  action_block action;
  action.active = true;
  action.align = m_opts.align;
  action.button = btn;
  action.start_x = m_xpos;
  action.command = string_util::replace_all(cmd, ":", "\\:");
  m_actions.emplace_back(action);
}  // }}}

/**
 * Handle action block end
 */
void bar::on_action_block_close(mousebtn btn) {  // {{{
  m_log.trace_x("bar: action_block_close(%i)", static_cast<int>(btn));

  for (auto i = m_actions.size(); i > 0; i--) {
    auto& action = m_actions[i - 1];

    if (!action.active || action.button != btn)
      continue;

    action.active = false;

    if (action.align == alignment::LEFT) {
      action.end_x = m_xpos;
    } else if (action.align == alignment::CENTER) {
      int base_x = m_opts.width;
      base_x -= m_borders[border::RIGHT].size;
      base_x /= 2;
      base_x += m_borders[border::LEFT].size;

      int clickable_width = m_xpos - action.start_x;
      action.start_x = base_x - clickable_width / 2 + action.start_x / 2;
      action.end_x = action.start_x + clickable_width;
    } else if (action.align == alignment::RIGHT) {
      int base_x = m_opts.width - m_borders[border::RIGHT].size;
      action.start_x = base_x - m_xpos + action.start_x;
      action.end_x = base_x;
    }

    return;
  }
}  // }}}

/**
 * Handle color change
 */
void bar::on_color_change(gc gc_, color color_) {  // {{{
  m_log.trace_x(
      "bar: color_change(%i, %s -> #%07x)", static_cast<int>(gc_), color_.source(), static_cast<uint32_t>(color_));

  const uint32_t value_list[]{color_};
  m_connection.change_gc(m_gcontexts.at(gc_), XCB_GC_FOREGROUND, value_list);

  if (gc_ == gc::FG) {
    m_fontmanager->allocate_color(color_);
  } else if (gc_ == gc::BG) {
    draw_shift(m_xpos, 0);
  }
}  // }}}

/**
 * Handle font change
 */
void bar::on_font_change(int index) {  // {{{
  m_log.trace_x("bar: font_change(%i)", index);
  m_fontmanager->set_preferred_font(index);
}  // }}}

/**
 * Handle pixel offsetting
 */
void bar::on_pixel_offset(int px) {  // {{{
  m_log.trace_x("bar: pixel_offset(%i)", px);
  draw_shift(m_xpos, px);
  m_xpos += px;
}  // }}}

/**
 * Proess systray report
 */
void bar::on_tray_report(uint16_t slots) {  // {{{
  if (m_trayclients == slots) {
    return;
  }

  m_log.trace("bar: tray_report(%lu)", slots);
  m_trayclients = slots;

  if (!m_prevdata.empty()) {
    parse(m_prevdata, true);
  }
}  // }}}

/**
 * Draw background onto the pixmap
 */
void bar::draw_background() {  // {{{
  draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BG), 0, 0, m_opts.width, m_opts.height);
}  // }}}

/**
 * Draw borders onto the pixmap
 */
void bar::draw_border(border border_) {  // {{{
  switch (border_) {
    case border::NONE:
      break;

    case border::TOP:
      if (m_borders[border::TOP].size > 0) {
        draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BT), m_borders[border::LEFT].size, 0,
            m_opts.width - m_borders[border::LEFT].size - m_borders[border::RIGHT].size, m_borders[border::TOP].size);
      }
      break;

    case border::BOTTOM:
      if (m_borders[border::BOTTOM].size > 0) {
        draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BB), m_borders[border::LEFT].size,
            m_opts.height - m_borders[border::BOTTOM].size,
            m_opts.width - m_borders[border::LEFT].size - m_borders[border::RIGHT].size,
            m_borders[border::BOTTOM].size);
      }
      break;

    case border::LEFT:
      if (m_borders[border::LEFT].size > 0) {
        draw_util::fill(
            m_connection, m_pixmap, m_gcontexts.at(gc::BL), 0, 0, m_borders[border::LEFT].size, m_opts.height);
      }
      break;

    case border::RIGHT:
      if (m_borders[border::RIGHT].size > 0) {
        draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BR), m_opts.width - m_borders[border::RIGHT].size, 0,
            m_borders[border::RIGHT].size, m_opts.height);
      }
      break;

    case border::ALL:
      draw_border(border::TOP);
      draw_border(border::BOTTOM);
      draw_border(border::LEFT);
      draw_border(border::RIGHT);
      break;
  }
}  // }}}

/**
 * Draw over- and underline onto the pixmap
 */
void bar::draw_lines(int x, int w) {  // {{{
  if (!m_opts.lineheight)
    return;

  if (m_attributes & static_cast<int>(attribute::o))
    draw_util::fill(
        m_connection, m_pixmap, m_gcontexts.at(gc::OL), x, m_borders[border::TOP].size, w, m_opts.lineheight);

  if (m_attributes & static_cast<int>(attribute::u))
    draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::UL), x,
        m_opts.height - m_borders[border::BOTTOM].size - m_opts.lineheight, w, m_opts.lineheight);
}  // }}}

/**
 * Shift the contents of the pixmap horizontally
 */
int bar::draw_shift(int x, int chr_width) {  // {{{
  int delta = chr_width;

  if (m_opts.align == alignment::CENTER) {
    int base_x = m_opts.width;
    base_x -= m_borders[border::RIGHT].size;
    base_x /= 2;
    base_x += m_borders[border::LEFT].size;
    m_connection.copy_area(m_pixmap, m_pixmap, m_gcontexts.at(gc::FG), base_x - x / 2, 0, base_x - (x + chr_width) / 2,
        0, x, m_opts.height);
    x = base_x - (x + chr_width) / 2 + x;
    delta /= 2;
  } else if (m_opts.align == alignment::RIGHT) {
    m_connection.copy_area(m_pixmap, m_pixmap, m_gcontexts.at(gc::FG), m_opts.width - x, 0,
        m_opts.width - x - chr_width, 0, x, m_opts.height);
    x = m_opts.width - chr_width - m_borders[border::RIGHT].size;
  }

  draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BG), x, 0, m_opts.width - x, m_opts.height);

  // Translate pos of clickable areas
  if (m_opts.align != alignment::LEFT)
    for (auto&& action : m_actions) {
      if (action.active || action.align != m_opts.align)
        continue;
      action.start_x -= delta;
      action.end_x -= delta;
    }

  return x;
}  // }}}

/**
 * Draw text character
 */
void bar::draw_character(uint16_t character) {  // {{{
  auto& font = m_fontmanager->match_char(character);
  if (!font) {
    return;
  }

  if (font->ptr && font->ptr != m_gcfont) {
    m_gcfont = font->ptr;
    m_fontmanager->set_gcontext_font(m_gcontexts.at(gc::FG), m_gcfont);
  }

  auto chr_width = m_fontmanager->char_width(font, character);

  // Avoid odd glyph width's for center-aligned text
  // since it breaks the positioning of clickable area's
  if (m_opts.align == alignment::CENTER && chr_width % 2)
    chr_width++;

  auto x = draw_shift(m_xpos, chr_width);
  auto y = m_opts.vertical_mid + font->height / 2 - font->descent + font->offset_y;

  // m_log.trace("Draw char(%c, width: %i) at pos(%i,%i)", character, chr_width, x, y);

  if (font->xft != nullptr) {
    auto color = m_fontmanager->xftcolor();
    XftDrawString16(m_xftdraw, &color, font->xft, x, y, &character, 1);
  } else {
    uint16_t ucs = ((character >> 8) | (character << 8));
    draw_util::xcb_poly_text_16_patched(m_connection, m_pixmap, m_gcontexts.at(gc::FG), x, y, 1, &ucs);
  }

  draw_lines(x, chr_width);
  m_xpos += chr_width;
}  // }}}

/**
 * Draw text string
 */
void bar::draw_textstring(const char* text, size_t len) {  // {{{
  for (size_t n = 0; n < len; n++) {
    vector<uint16_t> chars;
    chars.emplace_back(text[n]);

    auto& font = m_fontmanager->match_char(chars[0]);

    if (!font) {
      return;
    }

    if (font->ptr && font->ptr != m_gcfont) {
      m_gcfont = font->ptr;
      m_fontmanager->set_gcontext_font(m_gcontexts.at(gc::FG), m_gcfont);
    }

    while (n + 1 < len && text[n + 1] == chars[0]) {
      chars.emplace_back(text[++n]);
    }

    // TODO: cache
    auto chr_width = m_fontmanager->char_width(font, chars[0]) * chars.size();

    // Avoid odd glyph width's for center-aligned text
    // since it breaks the positioning of clickable area's
    if (m_opts.align == alignment::CENTER && chr_width % 2)
      chr_width++;

    auto x = draw_shift(m_xpos, chr_width);
    auto y = m_opts.vertical_mid + font->height / 2 - font->descent + font->offset_y;

    // m_log.trace("Draw char(%c, width: %i) at pos(%i,%i)", character, chr_width, x, y);

    if (font->xft != nullptr) {
      auto color = m_fontmanager->xftcolor();
      const FcChar16* drawchars = static_cast<const FcChar16*>(chars.data());
      XftDrawString16(m_xftdraw, &color, font->xft, x, y, drawchars, chars.size());
    } else {
      for (size_t i = 0; i < chars.size(); i++) {
        chars[i] = ((chars[i] >> 8) | (chars[i] << 8));
      }

      draw_util::xcb_poly_text_16_patched(
          m_connection, m_pixmap, m_gcontexts.at(gc::FG), x, y, chars.size(), chars.data());
    }

    draw_lines(x, chr_width);
    m_xpos += chr_width;
  }
}  // }}}

LEMONBUDDY_NS_END
