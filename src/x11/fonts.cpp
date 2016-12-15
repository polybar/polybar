#include <X11/Xlib-xcb.h>

#include "components/logger.hpp"
#include "errors.hpp"
#include "utils/color.hpp"
#include "utils/factory.hpp"
#include "utils/memory.hpp"
#include "x11/connection.hpp"
#include "x11/draw.hpp"
#include "x11/fonts.hpp"
#include "x11/xlib.hpp"
#include "x11/xutils.hpp"

POLYBAR_NS

void font_ref::_deleter::operator()(font_ref* font) {
  font->glyph_widths.clear();
  font->width_lut.clear();

  if (font->xft != nullptr) {
    XftFontClose(xlib::get_display().get(), font->xft);
  }
  if (font->ptr != XCB_NONE) {
    xcb_close_font(xutils::get_connection().get(), font->ptr);
  }
  delete font;
}

/**
 * Create instance
 */
font_manager::make_type font_manager::make() {
  return factory_util::unique<font_manager>(
      connection::make(), logger::make(), xlib::get_display(), xlib::get_visual(), xlib::create_colormap());
}

font_manager::font_manager(
    connection& conn, const logger& logger, shared_ptr<Display>&& dsp, shared_ptr<Visual>&& vis, Colormap&& cm)
    : m_connection(conn)
    , m_logger(logger)
    , m_display(forward<decltype(dsp)>(dsp))
    , m_visual(forward<decltype(vis)>(vis))
    , m_colormap(forward<decltype(cm)>(cm)) {
  if (!XftInit(nullptr) || !XftInitFtLibrary()) {
    throw application_error("Could not initialize Xft library");
  }
}

font_manager::~font_manager() {
  cleanup();
  if (m_display) {
    if (m_xftcolor_allocated) {
      XftColorFree(m_display.get(), m_visual.get(), m_colormap, &m_xftcolor);
    }
    XFreeColormap(m_display.get(), m_colormap);
  }
}

void font_manager::set_visual(shared_ptr<Visual>&& v) {
  m_visual = forward<decltype(v)>(v);
}

void font_manager::cleanup() {
  if (m_xftdraw != nullptr) {
    XftDrawDestroy(m_xftdraw);
    m_xftdraw = nullptr;
  }
}

bool font_manager::load(const string& name, int8_t fontindex, int8_t offset_y) {
  if (fontindex != DEFAULT_FONT_INDEX && m_fonts.find(fontindex) != m_fonts.end()) {
    m_logger.warn("A font with index '%i' has already been loaded, skip...", fontindex);
    return false;
  } else if (fontindex == DEFAULT_FONT_INDEX) {
    fontindex = m_fonts.size();
    m_logger.trace("font_manager: Assign font '%s' to index '%d'", name.c_str(), fontindex);
  } else {
    m_logger.trace("font_manager: Add font '%s' to index '%i'", name, fontindex);
  }

  shared_ptr<font_ref> font{new font_ref{}, font_ref::deleter};

  font->offset_y = offset_y;

  if (open_xcb_font(font, name)) {
    m_logger.info("Loaded font (xlfd=%s)", name);
  } else if (font->ptr != XCB_NONE) {
    m_connection.close_font_checked(font->ptr);
    font->ptr = XCB_NONE;
  }

  if (font->ptr == XCB_NONE &&
      (font->xft = XftFontOpenName(m_display.get(), m_connection.default_screen(), name.c_str())) != nullptr) {
    font->ascent = font->xft->ascent;
    font->descent = font->xft->descent;
    font->height = font->ascent + font->descent;

    if (font->xft->pattern != nullptr) {
      // XftChar8* file;
      // XftPatternGetString(font->xft->pattern, "file", 0, &file);
      // m_logger.info("Loaded font (pattern=%s, file=%s)", name, file);
      m_logger.info("Loaded font (pattern=%s)", name);
    } else {
      m_logger.info("Loaded font (pattern=%s)", name);
    }
  }

  if (font->ptr == XCB_NONE && font->xft == nullptr) {
    return false;
  }

  m_fonts.emplace(make_pair(fontindex, move(font)));

  int max_height{0};

  for (auto& iter : m_fonts) {
    if (iter.second->height > max_height) {
      max_height = iter.second->height;
    }
  }

  for (auto& iter : m_fonts) {
    iter.second->height = max_height;
  }

  return true;
}

void font_manager::set_preferred_font(int8_t index) {
  if (index <= 0) {
    m_fontindex = DEFAULT_FONT_INDEX;
    return;
  }

  for (auto&& font : m_fonts) {
    if (font.first == index) {
      m_fontindex = index;
      break;
    }
  }
}

shared_ptr<font_ref> font_manager::match_char(const uint16_t chr) {
  if (m_fonts.empty()) {
    return {};
  }

  if (m_fontindex != DEFAULT_FONT_INDEX && static_cast<size_t>(m_fontindex) <= m_fonts.size()) {
    auto iter = m_fonts.find(m_fontindex);
    if (iter == m_fonts.end() || !iter->second) {
      return {};
    } else if (has_glyph_xft(iter->second, chr)) {
      return iter->second;
    } else if (has_glyph_xcb(iter->second, chr)) {
      return iter->second;
    }
  }

  for (auto&& font : m_fonts) {
    if (!font.second) {
      return {};
    } else if (has_glyph_xft(font.second, chr)) {
      return font.second;
    } else if (has_glyph_xcb(font.second, chr)) {
      return font.second;
    }
  }

  return {};
}

uint8_t font_manager::glyph_width(const shared_ptr<font_ref>& font, const uint16_t chr) {
  if (font && font->xft != nullptr) {
    return glyph_width_xft(move(font), chr);
  } else if (font && font->ptr != XCB_NONE) {
    return glyph_width_xcb(move(font), chr);
  } else {
    return 0;
  }
}

void font_manager::drawtext(const shared_ptr<font_ref>& font, xcb_pixmap_t pm, xcb_gcontext_t gc, int16_t x, int16_t y,
    const uint16_t* chars, size_t num_chars) {
  if (m_xftdraw == nullptr) {
    m_xftdraw = XftDrawCreate(m_display.get(), pm, m_visual.get(), m_colormap);
  }
  if (font->xft != nullptr) {
    XftDrawString16(m_xftdraw, &m_xftcolor, font->xft, x, y, chars, num_chars);
  } else if (font->ptr != XCB_NONE) {
    uint16_t* ucs = static_cast<uint16_t*>(calloc(num_chars, sizeof(uint16_t)));
    for (size_t i = 0; i < num_chars; i++) {
      ucs[i] = ((chars[i] >> 8) | (chars[i] << 8));
    }
    draw_util::xcb_poly_text_16_patched(m_connection, pm, gc, x, y, num_chars, ucs);
  }
}

void font_manager::allocate_color(uint32_t color) {
  // clang-format off
  XRenderColor x{
    color_util::red_channel<uint16_t>(color),
    color_util::green_channel<uint16_t>(color),
    color_util::blue_channel<uint16_t>(color),
    color_util::alpha_channel<uint16_t>(color)};
  // clang-format on
  allocate_color(x);
}

void font_manager::allocate_color(XRenderColor color) {
  if (m_xftcolor_allocated) {
    XftColorFree(m_display.get(), m_visual.get(), m_colormap, &m_xftcolor);
  }

  if (!(m_xftcolor_allocated = XftColorAllocValue(m_display.get(), m_visual.get(), m_colormap, &color, &m_xftcolor))) {
    m_logger.err("Failed to allocate color");
  }
}

void font_manager::set_gcontext_font(const shared_ptr<font_ref>& font, xcb_gcontext_t gc, xcb_font_t* xcb_font) {
  const uint32_t val[1]{*xcb_font};
  m_connection.change_gc(gc, XCB_GC_FONT, val);
  *xcb_font = font->ptr;
}

bool font_manager::open_xcb_font(const shared_ptr<font_ref>& font, string fontname) {
  try {
    uint32_t font_id{m_connection.generate_id()};
    m_connection.open_font_checked(font_id, fontname);

    m_logger.trace("Found X font '%s'", fontname);
    font->ptr = font_id;

    auto query = m_connection.query_font(font_id);
    if (query->char_infos_len == 0) {
      m_logger.warn("X font '%s' does not contain any characters... (Verify the XLFD string)", fontname);
      return false;
    }

    font->descent = query->font_descent;
    font->height = query->font_ascent + query->font_descent;
    font->width = query->max_bounds.character_width;
    font->char_max = query->max_byte1 << 8 | query->max_char_or_byte2;
    font->char_min = query->min_byte1 << 8 | query->min_char_or_byte2;

    auto chars = query.char_infos();
    for (auto it = chars.begin(); it != chars.end(); it++) {
      font->width_lut.emplace_back(forward<xcb_charinfo_t>(*it));
    }
    return true;
  } catch (const std::exception& e) {
    m_logger.trace("font_manager: Could not find X font '%s' (what: %s)", fontname, e.what());
  }

  return false;
}

uint8_t font_manager::glyph_width_xft(const shared_ptr<font_ref>& font, const uint16_t chr) {
  auto it = font->glyph_widths.find(chr);
  if (it != font->glyph_widths.end()) {
    return it->second;
  }

  XGlyphInfo extents{};
  FT_UInt glyph{XftCharIndex(m_display.get(), font->xft, static_cast<FcChar32>(chr))};

  XftFontLoadGlyphs(m_display.get(), font->xft, FcFalse, &glyph, 1);
  XftGlyphExtents(m_display.get(), font->xft, &glyph, 1, &extents);
  XftFontUnloadGlyphs(m_display.get(), font->xft, &glyph, 1);

  font->glyph_widths.emplace_hint(it, chr, extents.xOff);  //.emplace_back(chr, extents.xOff);

  return extents.xOff;
}

uint8_t font_manager::glyph_width_xcb(const shared_ptr<font_ref>& font, const uint16_t chr) {
  if (!font || font->ptr == XCB_NONE) {
    return 0;
  } else if (static_cast<size_t>(chr - font->char_min) < font->width_lut.size()) {
    return font->width_lut[chr - font->char_min].character_width;
  } else {
    return font->width;
  }
}

bool font_manager::has_glyph_xft(const shared_ptr<font_ref>& font, const uint16_t chr) {
  if (!font || font->xft == nullptr) {
    return false;
  } else if (XftCharExists(m_display.get(), font->xft, static_cast<FcChar32>(chr)) == FcFalse) {
    return false;
  } else {
    return true;
  }
}

bool font_manager::has_glyph_xcb(const shared_ptr<font_ref>& font, const uint16_t chr) {
  if (font->ptr == XCB_NONE) {
    return false;
  } else if (chr < font->char_min || chr > font->char_max) {
    return false;
  } else if (static_cast<size_t>(chr - font->char_min) >= font->width_lut.size()) {
    return false;
  } else if (font->width_lut[chr - font->char_min].character_width == 0) {
    return false;
  } else {
    return true;
  }
}

POLYBAR_NS_END
