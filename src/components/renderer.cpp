#include "components/renderer.hpp"
#include "components/logger.hpp"
#include "x11/connection.hpp"
#include "x11/draw.hpp"
#include "x11/fonts.hpp"
#include "x11/xlib.hpp"
#include "x11/xutils.hpp"

POLYBAR_NS

/**
 * Configure injection module
 */
di::injector<unique_ptr<renderer>> configure_renderer(const bar_settings& bar, const vector<string>& fonts) {
  // clang-format off
  return di::make_injector(
      di::bind<>().to(bar),
      di::bind<>().to(fonts),
      configure_connection(),
      configure_logger(),
      configure_font_manager());
  // clang-format on
}

/**
 * Construct renderer instance
 */
renderer::renderer(connection& conn, const logger& logger, unique_ptr<font_manager> font_manager,
    const bar_settings& bar, const vector<string>& fonts)
    : m_connection(conn), m_log(logger), m_fontmanager(forward<decltype(font_manager)>(font_manager)), m_bar(bar) {
  auto screen = m_connection.screen();

  m_log.trace("renderer: Get true color visual");
  m_visual = m_connection.visual_type(screen, 32).get();

  m_log.trace("renderer: Create colormap");
  m_colormap = m_connection.generate_id();
  m_connection.create_colormap(XCB_COLORMAP_ALLOC_NONE, m_colormap, screen->root, m_visual->visual_id);

  m_window = m_connection.generate_id();
  m_log.trace("renderer: Create window (xid=%s)", m_connection.id(m_window));
  {
    uint32_t mask{0};
    uint32_t values[16]{0};
    xcb_params_cw_t params;

    XCB_AUX_ADD_PARAM(&mask, &params, back_pixel, 0);
    XCB_AUX_ADD_PARAM(&mask, &params, border_pixel, 0);
    XCB_AUX_ADD_PARAM(&mask, &params, backing_store, XCB_BACKING_STORE_WHEN_MAPPED);
    XCB_AUX_ADD_PARAM(&mask, &params, colormap, m_colormap);
    XCB_AUX_ADD_PARAM(&mask, &params, override_redirect, m_bar.force_docking);
    XCB_AUX_ADD_PARAM(&mask, &params, event_mask,
        XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS);

    xutils::pack_values(mask, &params, values);
    m_connection.create_window(32, m_window, screen->root, m_bar.pos.x, m_bar.pos.y, m_bar.size.w, m_bar.size.h, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT, m_visual->visual_id, mask, values);
  }

  m_pixmap = m_connection.generate_id();
  m_log.trace("renderer: Create pixmap (xid=%s)", m_connection.id(m_pixmap));
  m_connection.create_pixmap(32, m_pixmap, m_window, m_bar.size.w, m_bar.size.h);

  m_log.trace("renderer: Create gcontexts");
  {
    // clang-format off
    vector<uint32_t> colors {
      m_bar.background,
      m_bar.foreground,
      m_bar.overline.color,
      m_bar.underline.color,
      m_bar.borders.at(edge::TOP).color,
      m_bar.borders.at(edge::BOTTOM).color,
      m_bar.borders.at(edge::LEFT).color,
      m_bar.borders.at(edge::RIGHT).color,
    };
    // clang-format on

    for (int i = 0; i < 8; i++) {
      uint32_t mask{0};
      uint32_t value_list[32]{0};
      xcb_params_gc_t params;

      XCB_AUX_ADD_PARAM(&mask, &params, foreground, colors[i]);
      XCB_AUX_ADD_PARAM(&mask, &params, graphics_exposures, 0);

      xutils::pack_values(mask, &params, value_list);
      m_gcontexts.emplace(gc(i), m_connection.generate_id());
      m_colors.emplace(gc(i), colors[i]);
      m_log.trace("renderer: Create gcontext (gc=%i, xid=%s)", i, m_connection.id(m_gcontexts.at(gc(i))));
      m_connection.create_gc(m_gcontexts.at(gc(i)), m_pixmap, mask, value_list);
    }
  }

  m_log.trace("renderer: Load fonts");
  {
    auto fonts_loaded = false;
    auto fontindex = 0;

    if (fonts.empty())
      m_log.warn("No fonts specified, using fallback font \"fixed\"");

    for (auto f : fonts) {
      fontindex++;
      vector<string> fd{string_util::split(f, ';')};
      string pattern{fd[0]};
      int offset{0};

      if (fd.size() > 1)
        offset = std::stoi(fd[1], 0, 10);

      if (m_fontmanager->load(pattern, fontindex, offset))
        fonts_loaded = true;
      else
        m_log.warn("Unable to load font '%s'", fd[0]);
    }

    if (!fonts_loaded && !fonts.empty())
      m_log.warn("Unable to load fonts, using fallback font \"fixed\"");

    if (!fonts_loaded && !m_fontmanager->load("fixed"))
      throw application_error("Unable to load fonts");

    m_fontmanager->allocate_color(m_bar.foreground, true);
  }
}

/**
 * Get output window
 */
xcb_window_t renderer::window() const {
  return m_window;
}

/**
 * Begin render routine
 */
void renderer::begin() {
  m_log.trace_x("renderer: begin");

#if DEBUG and DRAW_CLICKABLE_AREA_HINTS
  for (auto&& action : m_actions) {
    m_connection.destroy_window(action.hint);
  }
#endif

  m_currentx = m_bar.inner_area(true).x;
  m_attributes = 0;
  m_actions.clear();

  fill_border(m_bar.borders, edge::ALL);
  fill_background();

  m_fontmanager->create_xftdraw(m_pixmap, m_colormap);
}

/**
 * End render routine
 */
void renderer::end() {
  m_log.trace_x("renderer: end");

  redraw();
  m_fontmanager->destroy_xftdraw();

#ifdef DEBUG
  debughints();
#endif

  m_reserve = 0;
  m_reserve_at = edge::NONE;
}

/**
 * Redraw window contents
 */
void renderer::redraw() {
  m_log.info("Redrawing");

  xcb_rectangle_t r{0, 0, m_bar.size.w, m_bar.size.h};

  if (m_reserve_at == edge::LEFT) {
    r.x += m_reserve;
    r.width -= m_reserve;
  } else if (m_reserve_at == edge::RIGHT) {
    r.width -= m_reserve;
  }

  m_connection.copy_area(m_pixmap, m_window, m_gcontexts.at(gc::FG), r.x, r.y, r.x, r.y, r.width, r.height);
}

/**
 * Reserve space at given edge
 */
void renderer::reserve_space(edge side, uint16_t w) {
  m_log.trace_x("renderer: reserve_space(%i, %i)", static_cast<uint8_t>(side), w);
  m_reserve = w;
  m_reserve_at = side;
}

/**
 * Change value of background gc
 */
void renderer::set_background(const uint32_t color) {
  if (m_colors[gc::BG] == color) {
    return m_log.trace_x("renderer: ignoring unchanged background color(#%08x)", color);
  }
  m_log.trace_x("renderer: set_background(#%08x)", color);
  m_connection.change_gc(m_gcontexts.at(gc::BG), XCB_GC_FOREGROUND, &color);
  m_colors[gc::BG] = color;
  shift_content(0);
}

/**
 * Change value of foreground gc
 */
void renderer::set_foreground(const uint32_t color) {
  if (m_colors[gc::FG] == color) {
    return m_log.trace_x("renderer: ignoring unchanged foreground color(#%08x)", color);
  }
  m_log.trace_x("renderer: set_foreground(#%08x)", color);
  m_connection.change_gc(m_gcontexts.at(gc::FG), XCB_GC_FOREGROUND, &color);
  m_fontmanager->allocate_color(color);
  m_colors[gc::FG] = color;
}

/**
 * Change value of underline gc
 */
void renderer::set_underline(const uint32_t color) {
  if (m_colors[gc::UL] == color) {
    return m_log.trace_x("renderer: ignoring unchanged underline color(#%08x)", color);
  }
  m_log.trace_x("renderer: set_underline(#%08x)", color);
  m_connection.change_gc(m_gcontexts.at(gc::UL), XCB_GC_FOREGROUND, &color);
  m_colors[gc::UL] = color;
}

/**
 * Change value of overline gc
 */
void renderer::set_overline(const uint32_t color) {
  if (m_colors[gc::OL] == color) {
    return m_log.trace_x("renderer: ignoring unchanged overline color(#%08x)", color);
  }
  m_log.trace_x("renderer: set_overline(#%08x)", color);
  m_connection.change_gc(m_gcontexts.at(gc::OL), XCB_GC_FOREGROUND, &color);
  m_colors[gc::OL] = color;
}

/**
 * Change preferred font index used when matching glyphs
 */
void renderer::set_fontindex(const int8_t font) {
  if (m_fontindex == font) {
    return m_log.trace_x("renderer: ignoring unchanged font index(%i)", static_cast<int8_t>(font));
  }
  m_log.trace_x("renderer: set_fontindex(%i)", static_cast<int8_t>(font));
  m_fontmanager->set_preferred_font(font);
  m_fontindex = font;
}

/**
 * Change current alignment
 */
void renderer::set_alignment(const alignment align) {
  if (align == m_alignment) {
    return m_log.trace_x("renderer: ignoring unchanged alignment(%i)", static_cast<uint8_t>(align));
  }

  if (align == alignment::LEFT) {
    m_currentx = m_bar.borders.at(edge::LEFT).size;
  } else if (align == alignment::RIGHT) {
    m_currentx = m_bar.borders.at(edge::RIGHT).size;
  } else {
    m_currentx = 0;
  }

  if (align == alignment::LEFT && m_reserve_at == edge::LEFT) {
    m_currentx += m_reserve;
  } else if (align == alignment::RIGHT && m_reserve_at == edge::RIGHT) {
    m_currentx += m_reserve;
  }

  m_log.trace_x("renderer: set_alignment(%i)", static_cast<uint8_t>(align));
  m_alignment = align;
}

/**
 * Enable/remove attribute
 */
void renderer::set_attribute(const attribute attr, bool state) {
  m_log.trace_x("renderer: set_attribute(%i, %i)", static_cast<uint8_t>(attr), state);

  if (state) {
    m_attributes |= 1U << static_cast<uint8_t>(attr);
  } else {
    m_attributes &= ~(1U << static_cast<uint8_t>(attr));
  }
}

/**
 * Toggle attribute
 */
void renderer::toggle_attribute(const attribute attr) {
  m_log.trace_x("renderer: toggle_attribute(%i)", static_cast<uint8_t>(attr));
  m_attributes ^= 1U << static_cast<uint8_t>(attr);
}

/**
 * Check if the given attribute is set
 */
bool renderer::check_attribute(const attribute attr) {
  return (m_attributes >> static_cast<uint8_t>(attr)) & 1U;
}

/**
 * Fill background area
 */
void renderer::fill_background() {
  m_log.trace_x("renderer: fill_background");

  xcb_rectangle_t rect{0, 0, m_bar.size.w, m_bar.size.h};

  if (m_reserve_at == edge::LEFT) {
    rect.x += m_reserve;
    rect.width -= m_reserve;
  } else if (m_reserve_at == edge::RIGHT) {
    rect.width -= m_reserve;
  }

  draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BG), rect.x, rect.y, rect.width, rect.height);
}

/**
 * Fill border area
 */
void renderer::fill_border(const map<edge, border_settings>& borders, edge border) {
  m_log.trace_x("renderer: fill_border(%i)", static_cast<uint8_t>(border));

  for (auto&& b : borders) {
    if (!b.second.size || (border != edge::ALL && b.first != border))
      continue;

    switch (b.first) {
      case edge::TOP:
        draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BT), borders.at(edge::LEFT).size, 0,
            m_bar.size.w - borders.at(edge::LEFT).size - borders.at(edge::RIGHT).size, borders.at(edge::TOP).size);
        break;
      case edge::BOTTOM:
        draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BB), borders.at(edge::LEFT).size,
            m_bar.size.h - borders.at(edge::BOTTOM).size,
            m_bar.size.w - borders.at(edge::LEFT).size - borders.at(edge::RIGHT).size, borders.at(edge::BOTTOM).size);
        break;
      case edge::LEFT:
        draw_util::fill(
            m_connection, m_pixmap, m_gcontexts.at(gc::BL), 0, 0, borders.at(edge::LEFT).size, m_bar.size.h);
        break;
      case edge::RIGHT:
        draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BR), m_bar.size.w - borders.at(edge::RIGHT).size, 0,
            borders.at(edge::RIGHT).size, m_bar.size.h);
        break;

      default:
        break;
    }
  }
}

/**
 * Fill overline area
 */
void renderer::fill_overline(int16_t x, uint16_t w) {
  if (!check_attribute(attribute::OVERLINE)) {
    return m_log.trace_x("renderer: not filling overline (flag unset)");
  } else if (!m_bar.overline.size) {
    return m_log.trace_x("renderer: not filling overline (size=0)");
  }
  m_log.trace_x("renderer: fill_overline(%i, #%08x)", m_bar.overline.size, m_colors[gc::OL]);
  const xcb_rectangle_t inner{m_bar.inner_area(true)};
  draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::OL), x, inner.y, w, m_bar.overline.size);
}

/**
 * Fill underline area
 */
void renderer::fill_underline(int16_t x, uint16_t w) {
  if (!check_attribute(attribute::UNDERLINE)) {
    return m_log.trace_x("renderer: not filling underline (flag unset)");
  } else if (!m_bar.underline.size) {
    return m_log.trace_x("renderer: not filling underline (size=0)");
  }
  m_log.trace_x("renderer: fill_underline(%i, #%08x)", m_bar.underline.size, m_colors[gc::UL]);
  const xcb_rectangle_t inner{m_bar.inner_area(true)};
  int16_t y{static_cast<int16_t>(inner.height - m_bar.underline.size)};
  draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::UL), x, y, w, m_bar.underline.size);
}

/**
 * Shift filled area by given pixels
 */
void renderer::fill_shift(const int16_t px) {
  shift_content(px);
}

/**
 * Draw character glyph
 */
void renderer::draw_character(uint16_t character) {
  m_log.trace_x("renderer: draw_character(\"%c\")", character);

  auto& font = m_fontmanager->match_char(character);

  if (!font) {
    return m_log.warn("No suitable font found (character=%i)", character);
  }

  if (font->ptr && font->ptr != m_gcfont) {
    m_gcfont = font->ptr;
    m_fontmanager->set_gcontext_font(m_gcontexts.at(gc::FG), m_gcfont);
  }

  auto width = m_fontmanager->char_width(font, character);

  // Avoid odd glyph width's for center-aligned text
  // since it breaks the positioning of clickable area's
  if (m_alignment == alignment::CENTER && width % 2)
    width++;

  auto x = shift_content(width);
  auto y = m_bar.center.y + font->height / 2 - font->descent + font->offset_y;

  if (font->xft != nullptr) {
    auto color = m_fontmanager->xftcolor();
    XftDrawString16(m_fontmanager->xftdraw(), &color, font->xft, x, y, &character, 1);
  } else {
    uint16_t ucs = ((character >> 8) | (character << 8));
    draw_util::xcb_poly_text_16_patched(m_connection, m_pixmap, m_gcontexts.at(gc::FG), x, y, 1, &ucs);
  }
}

/**
 * Draw character glyphs
 */
void renderer::draw_textstring(const char* text, size_t len) {
  m_log.trace_x("renderer: draw_textstring(\"%s\")", text);

  for (size_t n = 0; n < len; n++) {
    vector<uint16_t> chars;
    chars.emplace_back(text[n]);

    auto& font = m_fontmanager->match_char(chars[0]);

    if (!font) {
      return m_log.warn("No suitable font found (character=%i)", chars[0]);
    }

    if (font->ptr && font->ptr != m_gcfont) {
      m_gcfont = font->ptr;
      m_fontmanager->set_gcontext_font(m_gcontexts.at(gc::FG), m_gcfont);
    }

    while (n + 1 < len && text[n + 1] == chars[0]) {
      chars.emplace_back(text[++n]);
    }

    // TODO: cache
    auto width = m_fontmanager->char_width(font, chars[0]) * chars.size();

    // Avoid odd glyph width's for center-aligned text
    // since it breaks the positioning of clickable area's
    if (m_alignment == alignment::CENTER && width % 2)
      width++;

    auto x = shift_content(width);
    auto y = m_bar.center.y + font->height / 2 - font->descent + font->offset_y;

    if (font->xft != nullptr) {
      auto color = m_fontmanager->xftcolor();
      const FcChar16* drawchars = static_cast<const FcChar16*>(chars.data());
      XftDrawString16(m_fontmanager->xftdraw(), &color, font->xft, x, y, drawchars, chars.size());
    } else {
      for (size_t i = 0; i < chars.size(); i++) {
        chars[i] = ((chars[i] >> 8) | (chars[i] << 8));
      }

      draw_util::xcb_poly_text_16_patched(
          m_connection, m_pixmap, m_gcontexts.at(gc::FG), x, y, chars.size(), chars.data());
    }
  }
}

/**
 * Create new action block at the current position
 */
void renderer::begin_action(const mousebtn btn, const string cmd) {
  action_block action{};
  action.button = btn;
  action.align = m_alignment;
  action.start_x = m_currentx;
  action.command = string_util::replace_all(cmd, ":", "\\:");
  action.active = true;
  if (action.button == mousebtn::NONE)
    action.button = mousebtn::LEFT;
  m_log.trace_x("renderer: begin_action(%i, %s)", static_cast<uint8_t>(action.button), cmd.c_str());
  m_actions.emplace_back(action);
}

/**
 * End action block at the current position
 */
void renderer::end_action(const mousebtn btn) {
  for (auto action = m_actions.rbegin(); action != m_actions.rend(); action++) {
    if (!action->active || action->button != btn)
      continue;

    m_log.trace_x("renderer: end_action(%i, %s)", static_cast<uint8_t>(btn), action->command.c_str());

    action->active = false;

    if (action->align == alignment::LEFT) {
      action->end_x = m_currentx;
    } else if (action->align == alignment::CENTER) {
      int base_x{m_bar.size.w};
      int clickable_width{m_currentx - action->start_x};
      base_x -= m_bar.borders.at(edge::RIGHT).size;
      base_x /= 2;
      base_x += m_bar.borders.at(edge::LEFT).size;
      action->start_x = base_x - clickable_width / 2 + action->start_x / 2;
      action->end_x = action->start_x + clickable_width;
    } else if (action->align == alignment::RIGHT) {
      int base_x{m_bar.size.w - m_bar.borders.at(edge::RIGHT).size};
      if (m_reserve_at == edge::RIGHT)
        base_x -= m_reserve;
      action->start_x = base_x - m_currentx + action->start_x;
      action->end_x = base_x;
    }

    return;
  }
}

/**
 * Get all action blocks
 */
const vector<action_block> renderer::get_actions() {
  return m_actions;
}

/**
 * Shift contents by given pixel value
 */
int16_t renderer::shift_content(const int16_t x, const int16_t shift_x) {
  m_log.trace_x("renderer: shift_content(%i)", shift_x);

  int delta = shift_x;
  int x2{x};

  if (m_alignment == alignment::CENTER) {
    int base_x = m_bar.size.w;
    base_x -= m_bar.borders.at(edge::RIGHT).size;
    base_x /= 2;
    base_x += m_bar.borders.at(edge::LEFT).size;
    m_connection.copy_area(
        m_pixmap, m_pixmap, m_gcontexts.at(gc::FG), base_x - x / 2, 0, base_x - (x + shift_x) / 2, 0, x, m_bar.size.h);
    x2 = base_x - (x + shift_x) / 2 + x;
    delta /= 2;
  } else if (m_alignment == alignment::RIGHT) {
    m_connection.copy_area(m_pixmap, m_pixmap, m_gcontexts.at(gc::FG), m_bar.size.w - x, 0, m_bar.size.w - x - shift_x,
        0, x, m_bar.size.h);
    x2 = m_bar.size.w - shift_x - m_bar.borders.at(edge::RIGHT).size;
    if (m_reserve_at == edge::RIGHT)
      x2 -= m_reserve;
  }

  draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BG), x2, 0, m_bar.size.w - x, m_bar.size.h);

  // Translate pos of clickable areas
  if (m_alignment != alignment::LEFT) {
    for (auto&& action : m_actions) {
      if (action.active || action.align != m_alignment)
        continue;
      action.start_x -= delta;
      action.end_x -= delta;
    }
  }

  m_currentx += shift_x;

  fill_underline(x2, shift_x);
  fill_overline(x2, shift_x);

  return x2;
}

/**
 * Shift contents by given pixel value
 */
int16_t renderer::shift_content(const int16_t shift_x) {
  return shift_content(m_currentx, shift_x);
}

/**
 * Draw debugging hints onto the output window
 */
void renderer::debughints() {
#if DEBUG and DRAW_CLICKABLE_AREA_HINTS
  m_log.info("Drawing debug hints");

  map<alignment, int> hint_num{{
      {alignment::LEFT, 0}, {alignment::CENTER, 0}, {alignment::RIGHT, 0},
  }};

  for (auto&& action : m_actions) {
    if (action.active) {
      continue;
    }

    hint_num[action.align]++;

    auto x = action.start_x;
    auto y = m_bar.y + hint_num[action.align]++ * DRAW_CLICKABLE_AREA_HINTS_OFFSET_Y;
    auto w = action.end_x - action.start_x - 2;
    auto h = m_bar.size.h - 2;

    const uint32_t mask = XCB_CW_BORDER_PIXEL | XCB_CW_OVERRIDE_REDIRECT;
    const uint32_t border_color = hint_num[action.align] % 2 ? 0xff0000 : 0x00ff00;
    const uint32_t values[2]{border_color, true};

    action.hint = m_connection.generate_id();
    m_connection.create_window(m_screen->root_depth, action.hint, m_screen->root, x, y, w, h, 1,
        XCB_WINDOW_CLASS_INPUT_OUTPUT, m_screen->root_visual, mask, values);
    m_connection.map_window(action.hint);
  }
#endif
}

POLYBAR_NS_END
