#include "components/renderer.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "x11/connection.hpp"
#include "x11/draw.hpp"
#include "x11/fonts.hpp"
#include "x11/generic.hpp"
#include "x11/winspec.hpp"
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
    : m_connection(conn)
    , m_log(logger)
    , m_fontmanager(forward<decltype(font_manager)>(font_manager))
    , m_bar(bar)
    , m_rect(bar.inner_area()) {
  m_log.trace("renderer: Get TrueColor visual");
  m_visual = m_connection.visual_type(m_connection.screen(), 32).get();

  m_log.trace("renderer: Allocate colormap");
  m_colormap = m_connection.generate_id();
  m_connection.create_colormap(XCB_COLORMAP_ALLOC_NONE, m_colormap, m_connection.screen()->root, m_visual->visual_id);

  m_log.trace("renderer: Allocate output window");
  {
    // clang-format off
    m_window = winspec(m_connection)
      << cw_size(m_bar.size)
      << cw_pos(m_bar.pos)
      << cw_depth(32)
      << cw_visual(m_visual->visual_id)
      << cw_class(XCB_WINDOW_CLASS_INPUT_OUTPUT)
      << cw_params_back_pixel(0)
      << cw_params_border_pixel(0)
      << cw_params_backing_store(XCB_BACKING_STORE_WHEN_MAPPED)
      << cw_params_colormap(m_colormap)
      << cw_params_event_mask(XCB_EVENT_MASK_PROPERTY_CHANGE
                             |XCB_EVENT_MASK_EXPOSURE
                             |XCB_EVENT_MASK_BUTTON_PRESS)
      << cw_params_override_redirect(m_bar.override_redirect)
      << cw_flush(true);
    // clang-format on
  }

  m_log.trace("renderer: Allocate window pixmap");
  m_pixmap = m_connection.generate_id();
  m_connection.create_pixmap(32, m_pixmap, m_window, m_rect.width, m_rect.height);

  m_log.trace("renderer: Allocate graphic contexts");
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

      m_colors.emplace(gc(i), colors[i]);
      m_gcontexts.emplace(gc(i), m_connection.generate_id());
      m_connection.create_gc(m_gcontexts.at(gc(i)), m_pixmap, mask, value_list);
    }
  }

  m_log.trace("renderer: Load fonts");
  {
    auto fonts_loaded = false;
    auto fontindex = 0;

    if (fonts.empty()) {
      m_log.warn("No fonts specified, using fallback font \"fixed\"");
    }

    for (const auto& f : fonts) {
      fontindex++;
      vector<string> fd{string_util::split(f, ';')};
      string pattern{fd[0]};
      int offset{0};

      if (fd.size() > 1) {
        offset = std::stoi(fd[1], nullptr, 10);
      }

      if (m_fontmanager->load(pattern, fontindex, offset)) {
        fonts_loaded = true;
      } else {
        m_log.warn("Unable to load font '%s'", fd[0]);
      }
    }

    if (!fonts_loaded && !fonts.empty()) {
      m_log.warn("Unable to load fonts, using fallback font \"fixed\"");
    }

    if (!fonts_loaded && !m_fontmanager->load("fixed")) {
      throw application_error("Unable to load fonts");
    }

    m_fontmanager->allocate_color(m_bar.foreground, true);
  }
}

/**
 * Deconstruct instance
 */
renderer::~renderer() {
  if (m_window != XCB_NONE) {
    m_connection.destroy_window(m_window);
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

  m_rect = m_bar.inner_area();
  m_alignment = alignment::NONE;
  m_currentx = 0;
  m_attributes = 0;
  m_actions.clear();

  m_fontmanager->create_xftdraw(m_pixmap, m_colormap);
}

/**
 * End render routine
 */
void renderer::end() {
  m_log.trace_x("renderer: end");

  m_fontmanager->destroy_xftdraw();

#ifdef DEBUG_HINTS
  debug_hints();
#endif

  flush(false);
}

/**
 * Redraw window contents
 */
void renderer::flush(bool clear) {
  const xcb_rectangle_t& r = m_rect;

  xcb_rectangle_t top{0, 0, 0U, 0U};
  top.x += m_bar.borders.at(edge::LEFT).size;
  top.width += m_bar.size.w - m_bar.borders.at(edge::LEFT).size - m_bar.borders.at(edge::RIGHT).size;
  top.height += m_bar.borders.at(edge::TOP).size;

  xcb_rectangle_t bottom{0, 0, 0U, 0U};
  bottom.x += m_bar.borders.at(edge::LEFT).size;
  bottom.y += m_bar.size.h - m_bar.borders.at(edge::BOTTOM).size;
  bottom.width += m_bar.size.w - m_bar.borders.at(edge::LEFT).size - m_bar.borders.at(edge::RIGHT).size;
  bottom.height += m_bar.borders.at(edge::BOTTOM).size;

  xcb_rectangle_t left{0, 0, 0U, 0U};
  left.width += m_bar.borders.at(edge::LEFT).size;
  left.height += m_bar.size.h;

  xcb_rectangle_t right{0, 0, 0U, 0U};
  right.x += m_bar.size.w - m_bar.borders.at(edge::RIGHT).size;
  right.width += m_bar.borders.at(edge::RIGHT).size;
  right.height += m_bar.size.h;

  // Calculate the area that was reserved so that we
  // can clear any previous content drawn at the same location
  xcb_rectangle_t clear_area{0, 0, 0U, 0U};

  if (m_cleararea.size && m_cleararea.side == edge::RIGHT) {
    clear_area.x = m_bar.size.w - m_cleararea.size;
    clear_area.y = 0;
    clear_area.width = m_cleararea.size;
    clear_area.height = m_bar.size.h;
  } else if (m_cleararea.size && m_cleararea.side == edge::LEFT) {
    clear_area.x = m_rect.x;
    clear_area.y = m_rect.y;
    clear_area.width = m_cleararea.size;
    clear_area.height = m_rect.height;
  } else if (m_cleararea.size && m_cleararea.side == edge::TOP) {
    clear_area.x = m_rect.x;
    clear_area.y = m_rect.y;
    clear_area.width = m_rect.width;
    clear_area.height = m_cleararea.size;
  } else if (m_cleararea.size && m_cleararea.side == edge::TOP) {
    clear_area.x = m_rect.x;
    clear_area.y = m_rect.y + m_rect.height - m_cleararea.size;
    clear_area.width = m_rect.width;
    clear_area.height = m_cleararea.size;
  }

  if (clear_area != m_cleared && clear_area != 0) {
    m_log.trace("renderer: clearing area %dx%d+%d+%d", clear_area.width, clear_area.height, clear_area.x, clear_area.y);
    m_connection.clear_area(0, m_window, clear_area.x, clear_area.y, clear_area.width, clear_area.height);
    m_cleared = clear_area;
  }

  m_log.trace("renderer: copy pixmap (clear=%i, geom=%dx%d+%d+%d)", clear, r.width, r.height, r.x, r.y);
  m_connection.copy_area(m_pixmap, m_window, m_gcontexts.at(gc::FG), 0, 0, r.x, r.y, r.width, r.height);

  m_log.trace_x("renderer: draw top border (%lupx, %08x)", top.height, m_bar.borders.at(edge::TOP).color);
  draw_util::fill(m_connection, m_window, m_gcontexts.at(gc::BT), top);

  m_log.trace_x("renderer: draw bottom border (%lupx, %08x)", bottom.height, m_bar.borders.at(edge::BOTTOM).color);
  draw_util::fill(m_connection, m_window, m_gcontexts.at(gc::BB), bottom);

  m_log.trace_x("renderer: draw left border (%lupx, %08x)", left.width, m_bar.borders.at(edge::LEFT).color);
  draw_util::fill(m_connection, m_window, m_gcontexts.at(gc::BL), left);

  m_log.trace_x("renderer: draw right border (%lupx, %08x)", right.width, m_bar.borders.at(edge::RIGHT).color);
  draw_util::fill(m_connection, m_window, m_gcontexts.at(gc::BR), right);

  if (clear) {
    m_connection.clear_area(false, m_pixmap, 0, 0, r.width, r.height);
  }

  m_connection.flush();
}

/**
 * Reserve space at given edge
 */
void renderer::reserve_space(edge side, uint16_t w) {
  m_log.trace_x("renderer: reserve_space(%i, %i)", static_cast<uint8_t>(side), w);

  m_cleararea.side = side;
  m_cleararea.size = w;

  switch (side) {
    case edge::NONE:
      break;
    case edge::TOP:
      m_rect.y += w;
      m_rect.height -= w;
      break;
    case edge::BOTTOM:
      m_rect.height -= w;
      break;
    case edge::LEFT:
      m_rect.x += w;
      m_rect.width -= w;
      break;
    case edge::RIGHT:
      m_rect.width -= w;
      break;
    case edge::ALL:
      m_rect.x += w;
      m_rect.y += w;
      m_rect.width -= w * 2;
      m_rect.height -= w * 2;
      break;
  }
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

  m_log.trace_x("renderer: set_alignment(%i)", static_cast<uint8_t>(align));
  m_alignment = align;
  m_currentx = 0;
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
  draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BG), 0, 0, m_rect.width, m_rect.height);
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
  draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::OL), x, 0, w, m_bar.overline.size);
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
  int16_t y{static_cast<int16_t>(m_rect.height - m_bar.underline.size)};
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
  m_log.trace_x("renderer: draw_character");

  auto& font = m_fontmanager->match_char(character);

  if (!font) {
    return m_log.warn("No suitable font found (character=%i)", character);
  }

  if (font->ptr && font->ptr != m_gcfont) {
    m_gcfont = font->ptr;
    m_fontmanager->set_gcontext_font(m_gcontexts.at(gc::FG), m_gcfont);
  }

  auto width = m_fontmanager->char_width(font, character);
  auto x = shift_content(width);
  auto y = m_rect.height / 2 + font->height / 2 - font->descent + font->offset_y;

  if (font->xft != nullptr) {
    auto color = m_fontmanager->xftcolor();
    XftDrawString16(m_fontmanager->xftdraw(), &color, font->xft, x, y, &character, 1);
  } else {
    uint16_t ucs = ((character >> 8) | (character << 8));
    draw_util::xcb_poly_text_16_patched(m_connection, m_pixmap, m_gcontexts.at(gc::FG), x, y, 1, &ucs);
  }

  fill_underline(x, width);
  fill_overline(x, width);
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
    auto x = shift_content(width);
    auto y = m_rect.height / 2 + font->height / 2 - font->descent + font->offset_y;

    if (font->xft != nullptr) {
      auto color = m_fontmanager->xftcolor();
      const FcChar16* drawchars = static_cast<const FcChar16*>(chars.data());
      XftDrawString16(m_fontmanager->xftdraw(), &color, font->xft, x, y, drawchars, chars.size());
    } else {
      for (unsigned short& i : chars) {
        i = ((i >> 8) | (i << 8));
      }

      draw_util::xcb_poly_text_16_patched(
          m_connection, m_pixmap, m_gcontexts.at(gc::FG), x, y, chars.size(), chars.data());
    }

    fill_underline(x, width);
    fill_overline(x, width);
  }
}

/**
 * Create new action block at the current position
 */
void renderer::begin_action(const mousebtn btn, const string& cmd) {
  action_block action{};
  action.button = btn;
  action.align = m_alignment;
  action.start_x = m_currentx;
  action.command = string_util::replace_all(cmd, ":", "\\:");
  action.active = true;
  if (action.button == mousebtn::NONE) {
    action.button = mousebtn::LEFT;
  }
  m_log.trace_x("renderer: begin_action(%i, %s)", static_cast<uint8_t>(action.button), cmd.c_str());
  m_actions.emplace_back(action);
}

/**
 * End action block at the current position
 */
void renderer::end_action(const mousebtn btn) {
  int16_t clickable_width{0};

  for (auto action = m_actions.rbegin(); action != m_actions.rend(); action++) {
    if (!action->active || action->align != m_alignment || action->button != btn) {
      continue;
    }

    action->active = false;

    switch (action->align) {
      case alignment::NONE:
        break;
      case alignment::LEFT:
        action->end_x = m_currentx;
        break;
      case alignment::CENTER:
        clickable_width = m_currentx - action->start_x;
        action->start_x = m_rect.width / 2 - clickable_width / 2 + action->start_x / 2;
        action->end_x = action->start_x + clickable_width;
        break;
      case alignment::RIGHT:
        action->start_x = m_rect.width - m_currentx + action->start_x;
        action->end_x = m_rect.width;
        break;
    }

    m_log.trace_x("renderer: end_action(%i, %s, %i)", static_cast<uint8_t>(btn), action->command, action->width());

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
int16_t renderer::shift_content(int16_t x, const int16_t shift_x) {
  m_log.trace_x("renderer: shift_content(%i)", shift_x);

  int16_t base_x{0};
  double delta{static_cast<double>(shift_x)};

  switch (m_alignment) {
    case alignment::NONE:
      break;
    case alignment::LEFT:
      break;
    case alignment::CENTER:
      base_x = static_cast<int16_t>(m_rect.width / 2);
      m_connection.copy_area(m_pixmap, m_pixmap, m_gcontexts.at(gc::FG), base_x - x / 2, 0, base_x - (x + shift_x) / 2,
          0, x, m_rect.height);
      x = base_x - (x + shift_x) / 2 + x;
      delta /= 2;
      break;
    case alignment::RIGHT:
      base_x = static_cast<int16_t>(m_rect.width - x);
      m_connection.copy_area(
          m_pixmap, m_pixmap, m_gcontexts.at(gc::FG), base_x, 0, base_x - shift_x, 0, x, m_rect.height);
      x = m_rect.width - shift_x;
      break;
  }

  draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BG), x, 0, m_rect.width - x, m_rect.height);

  // Translate pos of clickable areas
  if (m_alignment != alignment::LEFT) {
    for (auto&& action : m_actions) {
      if (action.active || action.align != m_alignment) {
        continue;
      }
      action.start_x -= delta;
      action.end_x -= delta;
    }
  }

  m_currentx += shift_x;

  return x;
}

/**
 * Shift contents by given pixel value
 */
int16_t renderer::shift_content(const int16_t shift_x) {
  return shift_content(m_currentx, shift_x);
}

#ifdef DEBUG_HINTS
/**
 * Draw debugging hints onto the output window
 */
void renderer::debug_hints() {
  uint16_t border_width{1};
  map<alignment, int> hint_num{{
      // clang-format off
      {alignment::LEFT, 0},
      {alignment::CENTER, 0},
      {alignment::RIGHT, 0},
      // clang-format on
  }};

  for (auto&& hintwin : m_debughints) {
    m_connection.destroy_window(hintwin);
  }

  m_debughints.clear();

  for (auto&& action : m_actions) {
    if (action.active) {
      continue;
    }

    uint8_t num{static_cast<uint8_t>(hint_num.find(action.align)->second++)};
    int16_t x{static_cast<int16_t>(m_bar.pos.x + m_rect.x + action.start_x)};
    int16_t y{static_cast<int16_t>(m_bar.pos.y + m_rect.y)};
    uint16_t w{static_cast<uint16_t>(action.width() - border_width * 2)};
    uint16_t h{static_cast<uint16_t>(m_rect.height - border_width * 2)};

    x += num * DEBUG_HINTS_OFFSET_X;
    y += num * DEBUG_HINTS_OFFSET_Y;

    xcb_window_t hintwin{m_connection.generate_id()};
    m_debughints.emplace_back(hintwin);

    // clang-format off
    winspec(m_connection, hintwin)
      << cw_size(w, h)
      << cw_pos(x, y)
      << cw_border(border_width)
      << cw_depth(32)
      << cw_visual(m_visual->visual_id)
      << cw_params_colormap(m_colormap)
      << cw_params_back_pixel(0)
      << cw_params_border_pixel(num % 2 ? 0xFFFF0000 : 0xFF00FF00)
      << cw_params_override_redirect(true)
      << cw_flush()
      ;
    // clang-format on

    xutils::compton_shadow_exclude(m_connection, hintwin);
    m_connection.map_window(hintwin);
    m_log.info("Debug hint created (x=%lu width=%lu)", x, w);
  }
}
#endif

POLYBAR_NS_END
