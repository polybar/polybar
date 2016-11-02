#include "x11/fontmanager.hpp"
#include "utils/memory.hpp"
#include "utils/color.hpp"

LEMONBUDDY_NS

array<char, XFT_MAXCHARS> xft_widths;
array<wchar_t, XFT_MAXCHARS> xft_chars;

fontmanager::fontmanager(connection& conn, const logger& logger)
    : m_connection(conn), m_logger(logger) {
  m_display = xlib::get_display();
  m_visual = xlib::get_visual(conn.default_screen());
  m_colormap = xlib::create_colormap(conn.default_screen());
}

fontmanager::~fontmanager() {
  XftColorFree(m_display, m_visual, m_colormap, &m_xftcolor);
  XFreeColormap(m_display, m_colormap);
  m_fonts.clear();
}

void fontmanager::set_preferred_font(int index) {
  if (index <= 0) {
    m_fontindex = -1;
    return;
  }

  for (auto&& font : m_fonts) {
    if (font.first == index) {
      m_fontindex = index;
      break;
    }
  }
}

bool fontmanager::load(string name, int fontindex, int offset_y) {
  if (fontindex != -1 && m_fonts.find(fontindex) != m_fonts.end()) {
    m_logger.warn("A font with index '%i' has already been loaded, skip...", fontindex);
    return false;
  } else if (fontindex == -1) {
    fontindex = m_fonts.size();
    m_logger.trace("fontmanager: Assign font '%s' to index '%d'", name.c_str(), fontindex);
  } else {
    m_logger.trace("fontmanager: Add font '%s' to index '%i'", name, fontindex);
  }

  m_fonts.emplace(make_pair(fontindex, font_t{new fonttype(), fonttype_deleter{}}));
  m_fonts[fontindex]->offset_y = offset_y;
  m_fonts[fontindex]->ptr = 0;
  m_fonts[fontindex]->xft = nullptr;

  if (open_xcb_font(m_fonts[fontindex], name)) {
    m_logger.trace("fontmanager: Successfully loaded X font '%s'", name);
  } else if ((m_fonts[fontindex]->xft = XftFontOpenName(m_display, 0, name.c_str())) != nullptr) {
    m_fonts[fontindex]->ptr = 0;
    m_fonts[fontindex]->ascent = m_fonts[fontindex]->xft->ascent;
    m_fonts[fontindex]->descent = m_fonts[fontindex]->xft->descent;
    m_fonts[fontindex]->height = m_fonts[fontindex]->ascent + m_fonts[fontindex]->descent;
    m_logger.trace("fontmanager: Successfully loaded Freetype font '%s'", name);
  } else {
    return false;
  }

  int max_height = 0;

  for (auto& iter : m_fonts)
    if (iter.second->height > max_height)
      max_height = iter.second->height;

  for (auto& iter : m_fonts) {
    iter.second->height = max_height;
  }

  return true;
}

font_t& fontmanager::match_char(uint16_t chr) {
  static font_t notfound;
  if (!m_fonts.empty()) {
    if (m_fontindex != -1 && size_t(m_fontindex) <= m_fonts.size()) {
      auto iter = m_fonts.find(m_fontindex);
      if (iter != m_fonts.end() && has_glyph(iter->second, chr))
        return iter->second;
    }
    for (auto& font : m_fonts) {
      if (has_glyph(font.second, chr))
        return font.second;
    }
  }
  return notfound;
}

int fontmanager::char_width(font_t& font, uint16_t chr) {
  if (!font)
    return 0;

  if (font->xft == nullptr) {
    if (static_cast<size_t>(chr - font->char_min) < font->width_lut.size())
      return font->width_lut[chr - font->char_min].character_width;
    else
      return font->width;
  }

  auto index = chr % XFT_MAXCHARS;
  while (xft_chars[index] != 0 && xft_chars[index] != chr) index = (index + 1) % XFT_MAXCHARS;

  if (!xft_chars[index]) {
    XGlyphInfo gi;
    FT_UInt glyph = XftCharIndex(m_display, font->xft, (FcChar32)chr);
    XftFontLoadGlyphs(m_display, font->xft, FcFalse, &glyph, 1);
    XftGlyphExtents(m_display, font->xft, &glyph, 1, &gi);
    XftFontUnloadGlyphs(m_display, font->xft, &glyph, 1);
    xft_chars[index] = chr;
    xft_widths[index] = gi.xOff;
    return gi.xOff;
  } else if (xft_chars[index] == chr) {
    return xft_widths[index];
  }

  return 0;
}

XftColor fontmanager::xftcolor() {
  return m_xftcolor;
}

void fontmanager::allocate_color(XRenderColor color, bool initial_alloc) {
  if (!initial_alloc)
    XftColorFree(m_display, m_visual, m_colormap, &m_xftcolor);

  if (!XftColorAllocValue(m_display, m_visual, m_colormap, &color, &m_xftcolor))
    m_logger.err("Failed to allocate color");
}

void fontmanager::set_gcontext_font(gcontext& gc, xcb_font_t font) {
  const uint32_t values[1]{font};
  m_connection.change_gc(gc, XCB_GC_FONT, values);
}

bool fontmanager::open_xcb_font(font_t& fontptr, string fontname) {
  try {
    font xfont(m_connection, m_connection.generate_id());

    m_connection.open_font_checked(xfont, fontname);
    m_logger.trace("Found X font '%s'", fontname);

    auto query = m_connection.query_font(xfont);
    if (query->char_infos_len == 0) {
      m_logger.warn(
          "X font '%s' does not contain any characters... (Verify the XLFD string)", fontname);
      return false;
    }

    fontptr->descent = query->font_descent;
    fontptr->height = query->font_ascent + query->font_descent;
    fontptr->width = query->max_bounds.character_width;
    fontptr->char_max = query->max_byte1 << 8 | query->max_char_or_byte2;
    fontptr->char_min = query->min_byte1 << 8 | query->min_char_or_byte2;

    auto chars = query.char_infos();
    for (auto it = chars.begin(); it != chars.end(); it++)
      fontptr->width_lut.emplace_back(forward<xcb_charinfo_t>(*it));

    fontptr->ptr = xfont;

    return true;
  } catch (const xpp::x::error::name& e) {
    m_logger.trace("fontmanager: Could not find X font '%s'", fontname);
  } catch (const shared_ptr<xcb_generic_error_t>& e) {
    m_logger.trace("fontmanager: Could not find X font '%s'", fontname);
  } catch (const std::exception& e) {
    m_logger.trace("fontmanager: Could not find X font '%s' (what: %s)", fontname, e.what());
  }

  return false;
}

bool fontmanager::has_glyph(font_t& font, uint16_t chr) {
  if (font->xft != nullptr) {
    return XftCharExists(m_display, font->xft, (FcChar32)chr) == true;
  } else {
     if (chr < font->char_min || chr > font->char_max)
      return false;
    if (static_cast<size_t>(chr - font->char_min) >= font->width_lut.size())
      return false;
    if (font->width_lut[chr - font->char_min].character_width == 0)
      return false;
    return true;
  }
}

LEMONBUDDY_NS_END
