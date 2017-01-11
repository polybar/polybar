#include "components/renderer.hpp"
#include "components/logger.hpp"
#include "errors.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "events/signal_receiver.hpp"
#include "utils/factory.hpp"
#include "utils/file.hpp"
#include "x11/connection.hpp"
#include "x11/draw.hpp"
#include "x11/extensions/all.hpp"
#include "x11/generic.hpp"
#include "x11/winspec.hpp"

POLYBAR_NS

/**
 * Create instance
 */
renderer::make_type renderer::make(const bar_settings& bar, vector<string>&& fonts) {
  // clang-format off
  return factory_util::unique<renderer>(
      connection::make(),
      signal_emitter::make(),
      logger::make(),
      font_manager::make(),
      forward<decltype(bar)>(bar),
      forward<decltype(fonts)>(fonts));
  // clang-format on
}

/**
 * Construct renderer instance
 */
renderer::renderer(connection& conn, signal_emitter& emitter, const logger& logger,
    unique_ptr<font_manager> font_manager, const bar_settings& bar, const vector<string>& fonts)
    : m_connection(conn)
    , m_sig(emitter)
    , m_log(logger)
    , m_fontmanager(forward<decltype(font_manager)>(font_manager))
    , m_bar(forward<const bar_settings&>(bar))
    , m_rect(m_bar.inner_area()) {
  m_sig.attach(this);

  m_log.trace("renderer: Get TrueColor visual");

  if ((m_visual = m_connection.visual_type(m_connection.screen(), 32)) == nullptr) {
    m_log.err("No 32-bit TrueColor visual found...");

    if ((m_visual = m_connection.visual_type(m_connection.screen(), 24)) == nullptr) {
      m_log.err("No 24-bit TrueColor visual found, aborting...");
      throw application_error("No matching TrueColor visual found...");
    }

    if (m_visual == nullptr) {
      throw application_error("No matching TrueColor visual found...");
    }

    m_depth = 24;

    m_fontmanager->set_visual(m_connection.visual(m_depth));
  }

  m_log.trace("renderer: Allocate colormap");
  m_colormap = m_connection.generate_id();
  m_connection.create_colormap(XCB_COLORMAP_ALLOC_NONE, m_colormap, m_connection.screen()->root, m_visual->visual_id);

  m_log.trace("renderer: Allocate output window");
  {
    // clang-format off
    m_window = winspec(m_connection)
      << cw_size(m_bar.size)
      << cw_pos(m_bar.pos)
      << cw_depth(m_depth)
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
  m_connection.create_pixmap(m_depth, m_pixmap, m_window, m_rect.width, m_rect.height);

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

      xcb_params_gc_t params{};
      XCB_AUX_ADD_PARAM(&mask, &params, foreground, colors[i]);
      XCB_AUX_ADD_PARAM(&mask, &params, graphics_exposures, 0);
      connection::pack_values(mask, &params, value_list);

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
    m_fontmanager->allocate_color(m_bar.foreground);
  }
}

/**
 * Deconstruct instance
 */
renderer::~renderer() {
  m_sig.detach(this);

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
}

/**
 * End render routine
 */
void renderer::end() {
  m_log.trace_x("renderer: end");

  m_fontmanager->cleanup();

#ifdef DEBUG_HINTS
  debug_hints();
#endif

  flush(false);
}

/**
 * Flush pixmap contents onto the target window
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
  xcb_rectangle_t clear_area{r.x, r.y, r.width, r.height};

  if (m_cleararea.size && m_cleararea.side == edge::RIGHT) {
    clear_area.x += r.width;
    clear_area.y = top.height;
    clear_area.width = m_cleararea.size;
  } else if (m_cleararea.size && m_cleararea.side == edge::LEFT) {
    clear_area.x = left.width;
    clear_area.y = top.height;
    clear_area.width = m_cleararea.size;
  } else if (m_cleararea.size && m_cleararea.side == edge::TOP) {
    clear_area.height = m_cleararea.size;
  } else if (m_cleararea.size && m_cleararea.side == edge::BOTTOM) {
    clear_area.y += r.height - m_cleararea.size;
    clear_area.height = m_cleararea.size;
  }

  if (clear_area != m_cleared && clear_area != 0) {
    m_log.trace("renderer: clearing area %dx%d+%d+%d", clear_area.width, clear_area.height, clear_area.x, clear_area.y);
    m_connection.clear_area(0, m_window, clear_area.x, clear_area.y, clear_area.width, clear_area.height);
    m_cleared = clear_area;
  }

#if DEBUG
  if (m_bar.shaded && m_bar.origin == edge::TOP) {
    m_log.trace("renderer: copy pixmap (shaded=1, clear=%i, geom=%dx%d+%d+%d)", clear, r.width, r.height, r.x, r.y);
    auto geom = m_connection.get_geometry(m_window);
    auto x1 = 0;
    auto y1 = r.height - m_bar.shade_size.h - r.y - geom->height;
    auto x2 = r.x;
    auto y2 = r.y;
    auto w = r.width;
    auto h = r.height - m_bar.shade_size.h + geom->height;
    m_connection.copy_area(m_pixmap, m_window, m_gcontexts.at(gc::FG), x1, y1, x2, y2, w, h);
    m_connection.flush();
    return;
  }
#endif

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
 * Check if given attribute is enabled
 */
bool renderer::check_attribute(const attribute attr) {
  return (m_attributes >> static_cast<uint8_t>(attr)) & 1U;
}

/**
 * Fill background color
 */
void renderer::fill_background() {
  m_log.trace_x("renderer: fill_background");
  draw_util::fill(m_connection, m_pixmap, m_gcontexts.at(gc::BG), 0, 0, m_rect.width, m_rect.height);
}

/**
 * Fill overline color
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
 * Fill underline color
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
 * @see shift_content
 */
void renderer::fill_shift(const int16_t px) {
  shift_content(px);
}

/**
 * Draw consecutive character glyphs
 */
void renderer::draw_textstring(const uint16_t* text, size_t len) {
  m_log.trace_x("renderer: draw_textstring(\"%s\")", text);

  for (size_t n = 0; n < len; n++) {
    vector<uint16_t> chars{text[n]};
    shared_ptr<font_ref> font{m_fontmanager->match_char(chars[0])};
    uint8_t width{static_cast<const uint8_t>(m_fontmanager->glyph_width(font, chars[0]) * chars.size())};

    if (!font) {
      m_log.warn("Could not find glyph for %i", chars[0]);
      continue;
    } else if (!width) {
      m_log.warn("Could not determine glyph width for %i", chars[0]);
      continue;
    }

    while (n + 1 < len && text[n + 1] == chars[0]) {
      chars.emplace_back(text[n++]);
    }

    width *= chars.size();
    auto x = shift_content(width);
    auto y = m_rect.height / 2 + font->height / 2 - font->descent + font->offset_y;

    if (font->ptr != XCB_NONE && m_gcfont != font->ptr) {
      const uint32_t v[1]{font->ptr};
      m_connection.change_gc(m_gcontexts.at(gc::FG), XCB_GC_FONT, v);
      m_gcfont = font->ptr;
    }

    m_fontmanager->drawtext(font, m_pixmap, m_gcontexts.at(gc::FG), x, y, chars.data(), chars.size());

    fill_underline(x, width);
    fill_overline(x, width);
  }
}

/**
 * Get completed action blocks
 */
const vector<action_block> renderer::get_actions() {
  return m_actions;
}

/**
 * Shift pixmap content by given value
 */
int16_t renderer::shift_content(int16_t x, const int16_t shift_x) {
  if (x > m_rect.width) {
    return m_rect.width;
  } else if (x < 0) {
    return 0;
  }

  m_log.trace_x("renderer: shift_content(%i)", shift_x);

  int16_t base_x{0};
  double delta{0.0};

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
      delta = static_cast<double>(shift_x) / 2;
      break;
    case alignment::RIGHT:
      base_x = static_cast<int16_t>(m_rect.width - x);
      m_connection.copy_area(
          m_pixmap, m_pixmap, m_gcontexts.at(gc::FG), base_x, 0, base_x - shift_x, 0, x, m_rect.height);
      x = m_rect.width - shift_x;
      delta = static_cast<double>(shift_x);
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
 * @see shift_content
 */
int16_t renderer::shift_content(const int16_t shift_x) {
  return shift_content(m_currentx, shift_x);
}

#ifdef DEBUG_HINTS
/**
 * Draw boxes at the location of each created action block
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

  m_debughints.clear();

  for (auto&& action : m_actions) {
    if (action.active) {
      continue;
    }

    xcb_window_t hintwin{m_connection.generate_id()};
    m_debughints.emplace_back(hintwin);

    uint8_t num{static_cast<uint8_t>(hint_num.find(action.align)->second++)};

    // clang-format off
    winspec(m_connection, hintwin)
      << cw_size(action.width() - border_width * 2, m_rect.height - border_width * 2)
      << cw_pos(action.start_x + num * DEBUG_HINTS_OFFSET_X, m_bar.pos.y + m_rect.y + num * DEBUG_HINTS_OFFSET_Y)
      << cw_border(border_width)
      << cw_depth(m_depth)
      << cw_visual(m_visual->visual_id)
      << cw_params_colormap(m_colormap)
      << cw_params_back_pixel(0)
      << cw_params_border_pixel(num % 2 ? 0xFFFF0000 : 0xFF00FF00)
      << cw_params_override_redirect(true)
      << cw_flush()
      ;
    // clang-format on

    const uint32_t shadow{0};
    m_connection.change_property(XCB_PROP_MODE_REPLACE, hintwin, _COMPTON_SHADOW, XCB_ATOM_CARDINAL, 32, 1, &shadow);
    m_connection.map_window(hintwin);
  }
}
#endif

bool renderer::on(const change_background& evt) {
  const uint32_t color{evt.cast()};

  if (m_colors[gc::BG] == color) {
    m_log.trace_x("renderer: ignoring unchanged background color(#%08x)", color);
  } else {
    m_log.trace_x("renderer: set_background(#%08x)", color);
    m_connection.change_gc(m_gcontexts.at(gc::BG), XCB_GC_FOREGROUND, &color);
    m_colors[gc::BG] = color;
    shift_content(0);
  }

  return true;
}

bool renderer::on(const change_foreground& evt) {
  const uint32_t color{evt.cast()};

  if (m_colors[gc::FG] == color) {
    m_log.trace_x("renderer: ignoring unchanged foreground color(#%08x)", color);
  } else {
    m_log.trace_x("renderer: set_foreground(#%08x)", color);
    m_connection.change_gc(m_gcontexts.at(gc::FG), XCB_GC_FOREGROUND, &color);
    m_fontmanager->allocate_color(color);
    m_colors[gc::FG] = color;
  }

  return true;
}

bool renderer::on(const change_underline& evt) {
  const uint32_t color{evt.cast()};

  if (m_colors[gc::UL] == color) {
    m_log.trace_x("renderer: ignoring unchanged underline color(#%08x)", color);
  } else {
    m_log.trace_x("renderer: set_underline(#%08x)", color);
    m_connection.change_gc(m_gcontexts.at(gc::UL), XCB_GC_FOREGROUND, &color);
    m_colors[gc::UL] = color;
  }

  return true;
}

bool renderer::on(const change_overline& evt) {
  const uint32_t color{evt.cast()};

  if (m_colors[gc::OL] == color) {
    m_log.trace_x("renderer: ignoring unchanged overline color(#%08x)", color);
  } else {
    m_log.trace_x("renderer: set_overline(#%08x)", color);
    m_connection.change_gc(m_gcontexts.at(gc::OL), XCB_GC_FOREGROUND, &color);
    m_colors[gc::OL] = color;
  }

  return true;
}

bool renderer::on(const change_font& evt) {
  const uint8_t font{evt.cast()};

  if (m_fontindex == font) {
    m_log.trace_x("renderer: ignoring unchanged font index(%i)", static_cast<uint8_t>(font));
  } else {
    m_log.trace_x("renderer: fontindex(%i)", static_cast<uint8_t>(font));
    m_fontmanager->fontindex(font);
    m_fontindex = font;
  }

  return true;
}

bool renderer::on(const change_alignment& evt) {
  auto align = static_cast<const alignment&>(evt.cast());

  if (align == m_alignment) {
    m_log.trace_x("renderer: ignoring unchanged alignment(%i)", static_cast<uint8_t>(align));
  } else {
    m_log.trace_x("renderer: set_alignment(%i)", static_cast<uint8_t>(align));
    m_alignment = align;
    m_currentx = 0;
  }

  return true;
}

bool renderer::on(const offset_pixel& evt) {
  shift_content(evt.cast());
  return true;
}

bool renderer::on(const attribute_set& evt) {
  m_log.trace_x("renderer: attribute_set(%i, %i)", static_cast<uint8_t>(evt.cast()), true);
  m_attributes |= 1U << static_cast<uint8_t>(evt.cast());
  return true;
}

bool renderer::on(const attribute_unset& evt) {
  m_log.trace_x("renderer: attribute_unset(%i, %i)", static_cast<uint8_t>(evt.cast()), true);
  m_attributes &= ~(1U << static_cast<uint8_t>(evt.cast()));
  return true;
}

bool renderer::on(const attribute_toggle& evt) {
  m_log.trace_x("renderer: attribute_toggle(%i)", static_cast<uint8_t>(evt.cast()));
  m_attributes ^= 1U << static_cast<uint8_t>(evt.cast());
  return true;
}

bool renderer::on(const action_begin& evt) {
  auto a = static_cast<const action&>(evt.cast());
  action_block action{};
  action.button = a.button;
  action.align = m_alignment;
  action.start_x = m_currentx;
  action.command = string_util::replace_all(a.command, ":", "\\:");
  action.active = true;

  if (action.button == mousebtn::NONE) {
    action.button = mousebtn::LEFT;
  }

  m_log.trace_x("renderer: action_begin(%i, %s)", static_cast<uint8_t>(a.button), a.command.c_str());
  m_actions.emplace_back(action);

  return true;
}

bool renderer::on(const action_end& evt) {
  auto btn = static_cast<const mousebtn&>(evt.cast());
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

    action->start_x += m_rect.x;
    action->end_x += m_rect.x;

    m_log.trace_x("renderer: action_end(%i, %s, %i)", static_cast<uint8_t>(btn), action->command, action->width());
  }

  return true;
}

bool renderer::on(const write_text_ascii& evt) {
  const uint16_t data[1]{evt.cast()};
  draw_textstring(data, 1);
  return true;
}

bool renderer::on(const write_text_unicode& evt) {
  const uint16_t data[1]{evt.cast()};
  draw_textstring(data, 1);
  return true;
}

bool renderer::on(const write_text_string& evt) {
  auto pkt = evt.cast();
  draw_textstring(pkt.data, pkt.length);
  return true;
}

POLYBAR_NS_END
