#include <X11/Xlib-xcb.h>

#include "components/logger.hpp"
#include "errors.hpp"
#include "utils/color.hpp"
#include "utils/factory.hpp"
#include "utils/memory.hpp"
#include "x11/connection.hpp"
#include "x11/fonts.hpp"
#include "x11/xlib.hpp"
#include "x11/xutils.hpp"

POLYBAR_NS

#define XFT_MAXCHARS (1 << 16)

array<char, XFT_MAXCHARS> g_xft_widths;
array<wchar_t, XFT_MAXCHARS> g_xft_chars;

/**
 * Create instance
 */
font_manager::make_type font_manager::make() {
  return factory_util::unique<font_manager>(
      connection::make(), logger::make(), xlib::get_display(), xlib::get_visual());
}

void fonttype_deleter::operator()(fonttype* f) {
  if (f->xft != nullptr) {
    XftFontClose(xlib::get_display().get(), f->xft);
    free(f->xft);
  }
  if (f->ptr != XCB_NONE) {
    connection::make().close_font(f->ptr);
  }
  delete f;
}

font_manager::font_manager(connection& conn, const logger& logger, shared_ptr<Display>&& dsp, shared_ptr<Visual>&& vis)
    : m_connection(conn)
    , m_logger(logger)
    , m_display(forward<decltype(dsp)>(dsp))
    , m_visual(forward<decltype(vis)>(vis)) {
  m_colormap = xlib::create_colormap(conn.default_screen());
}

font_manager::~font_manager() {
  if (m_display) {
    if (m_xftcolor != nullptr) {
      XftColorFree(m_display.get(), m_visual.get(), m_colormap, m_xftcolor);
      free(m_xftcolor);
    }
    destroy_xftdraw();
    XFreeColormap(m_display.get(), m_colormap);
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

  fonttype_pointer f{new fonttype_pointer::element_type{}, fonttype_deleter{}};
  f->offset_y = offset_y;

  if (open_xcb_font(f, name)) {
    m_logger.info("Loaded font (xlfd=%s)", name);
  } else if (f->ptr != XCB_NONE) {
    m_connection.close_font_checked(f->ptr);
    f->ptr = XCB_NONE;
  }

  if (f->ptr == XCB_NONE &&
      (f->xft = XftFontOpenName(m_display.get(), m_connection.default_screen(), name.c_str())) != nullptr) {
    f->ascent = f->xft->ascent;
    f->descent = f->xft->descent;
    f->height = f->ascent + f->descent;

    if (f->xft->pattern != nullptr) {
      FcChar8* file{nullptr};
      FcPatternGetString(f->xft->pattern, "file", 0, &file);
      m_logger.info("Loaded font (pattern=%s, file=%s)", name, file);
      free(file);
    } else {
      m_logger.info("Loaded font (pattern=%s)", name);
    }
  }

  if (f->ptr == XCB_NONE && f->xft == nullptr) {
    return false;
  }

  m_fonts.emplace(make_pair(fontindex, move(f)));

  int max_height = 0;

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

fonttype_pointer& font_manager::match_char(uint16_t chr) {
  static fonttype_pointer notfound;
  if (!m_fonts.empty()) {
    if (m_fontindex != DEFAULT_FONT_INDEX && size_t(m_fontindex) <= m_fonts.size()) {
      auto iter = m_fonts.find(m_fontindex);
      if (iter != m_fonts.end() && has_glyph(iter->second, chr)) {
        return iter->second;
      }
    }
    for (auto& font : m_fonts) {
      if (has_glyph(font.second, chr)) {
        return font.second;
      }
    }
  }
  return notfound;
}

uint8_t font_manager::char_width(fonttype_pointer& font, uint16_t chr) {
  if (!font) {
    return 0;
  }

  if (font->xft == nullptr) {
    if (static_cast<size_t>(chr - font->char_min) < font->width_lut.size()) {
      return font->width_lut[chr - font->char_min].character_width;
    } else {
      return font->width;
    }
  }

  auto index = chr % XFT_MAXCHARS;
  while (g_xft_chars[index] != 0 && g_xft_chars[index] != chr) {
    index = (index + 1) % XFT_MAXCHARS;
  }

  if (!g_xft_chars[index]) {
    XGlyphInfo gi;
    FT_UInt glyph = XftCharIndex(m_display.get(), font->xft, static_cast<FcChar32>(chr));
    XftFontLoadGlyphs(m_display.get(), font->xft, FcFalse, &glyph, 1);
    XftGlyphExtents(m_display.get(), font->xft, &glyph, 1, &gi);
    XftFontUnloadGlyphs(m_display.get(), font->xft, &glyph, 1);
    g_xft_chars[index] = chr;
    g_xft_widths[index] = gi.xOff;
    return gi.xOff;
  } else if (g_xft_chars[index] == chr) {
    return g_xft_widths[index];
  }

  return 0;
}

XftColor* font_manager::xftcolor() {
  return m_xftcolor;
}

XftDraw* font_manager::xftdraw() {
  return m_xftdraw;
}

void font_manager::create_xftdraw(xcb_pixmap_t pm) {
  destroy_xftdraw();
  m_xftdraw = XftDrawCreate(m_display.get(), pm, m_visual.get(), m_colormap);
}

void font_manager::destroy_xftdraw() {
  if (m_xftdraw != nullptr) {
    XftDrawDestroy(m_xftdraw);
    m_xftdraw = nullptr;
  }
}

void font_manager::allocate_color(uint32_t color) {
  XRenderColor x;
  x.red = color_util::red_channel<uint16_t>(color);
  x.green = color_util::green_channel<uint16_t>(color);
  x.blue = color_util::blue_channel<uint16_t>(color);
  x.alpha = color_util::alpha_channel<uint16_t>(color);
  allocate_color(x);
}

void font_manager::allocate_color(XRenderColor color) {
  if (m_xftcolor != nullptr) {
    XftColorFree(m_display.get(), m_visual.get(), m_colormap, m_xftcolor);
    free(m_xftcolor);
  }

  m_xftcolor = static_cast<XftColor*>(malloc(sizeof(XftColor)));

  if (!XftColorAllocValue(m_display.get(), m_visual.get(), m_colormap, &color, m_xftcolor)) {
    m_logger.err("Failed to allocate color");
  }
}

void font_manager::set_gcontext_font(xcb_gcontext_t gc, xcb_font_t font) {
  const uint32_t values[1]{font};
  m_connection.change_gc(gc, XCB_GC_FONT, values);
}

bool font_manager::open_xcb_font(fonttype_pointer& fontptr, string fontname) {
  try {
    uint32_t font_id{m_connection.generate_id()};
    m_connection.open_font_checked(font_id, fontname);

    m_logger.trace("Found X font '%s'", fontname);
    fontptr->ptr = font_id;

    auto query = m_connection.query_font(font_id);
    if (query->char_infos_len == 0) {
      m_logger.warn("X font '%s' does not contain any characters... (Verify the XLFD string)", fontname);
      return false;
    }

    fontptr->descent = query->font_descent;
    fontptr->height = query->font_ascent + query->font_descent;
    fontptr->width = query->max_bounds.character_width;
    fontptr->char_max = query->max_byte1 << 8 | query->max_char_or_byte2;
    fontptr->char_min = query->min_byte1 << 8 | query->min_char_or_byte2;

    auto chars = query.char_infos();
    for (auto it = chars.begin(); it != chars.end(); it++) {
      fontptr->width_lut.emplace_back(forward<xcb_charinfo_t>(*it));
    }
    return true;
  } catch (const std::exception& e) {
    m_logger.trace("font_manager: Could not find X font '%s' (what: %s)", fontname, e.what());
  }

  return false;
}

bool font_manager::has_glyph(fonttype_pointer& font, uint16_t chr) {
  if (font->xft != nullptr) {
    return static_cast<bool>(XftCharExists(m_display.get(), font->xft, static_cast<FcChar32>(chr)));
  } else {
    if (chr < font->char_min || chr > font->char_max) {
      return false;
    }
    if (static_cast<size_t>(chr - font->char_min) >= font->width_lut.size()) {
      return false;
    }
    if (font->width_lut[chr - font->char_min].character_width == 0) {
      return false;
    }
    return true;
  }
}

POLYBAR_NS_END
