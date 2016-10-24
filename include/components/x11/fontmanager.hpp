#pragma once

#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcbext.h>
#include <map>
#include <string>

#include "common.hpp"
#include "components/logger.hpp"
#include "components/x11/color.hpp"
#include "components/x11/connection.hpp"
#include "components/x11/types.hpp"
#include "components/x11/xlib.hpp"
#include "utils/memory.hpp"
#include "utils/mixins.hpp"

LEMONBUDDY_NS

#define XFT_MAXCHARS (1 << 16)
static array<char, XFT_MAXCHARS> xft_widths;
static array<wchar_t, XFT_MAXCHARS> xft_chars;

struct fonttype {
  fonttype() {}
  XftFont* xft;
  xcb_font_t ptr;
  int offset_y = 0;
  int ascent = 0;
  int descent = 0;
  int height = 0;
  int width = 0;
  uint16_t char_max = 0;
  uint16_t char_min = 0;
  vector<xcb_charinfo_t> width_lut;
};

struct fonttype_deleter {
  void operator()(fonttype* f) {
    if (f->xft != nullptr)
      XftFontClose(xlib::get_display(), f->xft);
    else
      xcb_close_font(xutils::get_connection(), f->ptr);
  }
};

using font_t = unique_ptr<fonttype, fonttype_deleter>;

class fontmanager {
 public:
  explicit fontmanager(connection& conn, const logger& logger)
      : m_connection(conn), m_logger(logger) {
    m_display = xlib::get_display();
    m_visual = xlib::get_visual();
    m_colormap = XDefaultColormap(m_display, 0);
  }

  ~fontmanager() {
    XftColorFree(m_display, m_visual, m_colormap, &m_xftcolor);
    XFreeColormap(m_display, m_colormap);
    m_fonts.clear();
  }

  void set_preferred_font(int index) {  // {{{
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
  }  // }}}

  bool load(string name, int fontindex = -1, int offset_y = 0) {  // {{{
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
  }  // }}}

  font_t& match_char(uint16_t chr) {  // {{{
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
  }  // }}}

  int char_width(font_t& font, uint16_t chr) {  // {{{
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
  }  // }}}

  XftColor xftcolor() {  // {{{
    return m_xftcolor;
  }  // }}}

  void allocate_color(color xcolor, bool initial_alloc = false) {  // {{{
    if (!initial_alloc)
      XftColorFree(m_display, m_visual, m_colormap, &m_xftcolor);

    if (!XftColorAllocName(m_display, m_visual, m_colormap, xcolor.rgb().c_str(), &m_xftcolor))
      m_logger.err("Failed to allocate color '%s'", xcolor.hex());
  }  // }}}

  void set_gcontext_font(gcontext& gc, xcb_font_t font) {  // {{{
    const uint32_t values[1]{font};
    m_connection.change_gc(gc, XCB_GC_FONT, values);
  }  // }}}

 protected:
  bool open_xcb_font(font_t& fontptr, string fontname) {  // {{{
    try {
      font xfont(m_connection, m_connection.generate_id());

      m_connection.open_font_checked(xfont, fontname);
      m_logger.trace("Found X font '%s'", fontname);

      auto query = m_connection.query_font(xfont);
      fontptr->descent = query->font_descent;
      fontptr->height = query->font_ascent + query->font_descent;
      fontptr->width = query->max_bounds.character_width;
      fontptr->char_max = query->max_byte1 << 8 | query->max_char_or_byte2;
      fontptr->char_min = query->min_byte1 << 8 | query->min_char_or_byte2;

      if (query->char_infos_len > 0) {
        auto chars = query.char_infos();
        for (auto it = chars.begin(); it != chars.end(); it++)
          fontptr->width_lut.emplace_back(forward<xcb_charinfo_t>(*it));
      }

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
  }  // }}}

  bool has_glyph(font_t& font, uint16_t chr) {  // {{{
    if (font->xft != nullptr) {
      return XftCharExists(m_display, font->xft, (FcChar32)chr) == true;
    } else {
      if (chr < font->char_min || chr > font->char_max)
        return false;
      if (font->width_lut[chr - font->char_min].character_width == 0)
        return false;
      return true;
    }
  }  // }}}

 private:
  connection& m_connection;
  const logger& m_logger;

  Display* m_display = nullptr;
  Visual* m_visual = nullptr;
  Colormap m_colormap;

  map<int, font_t> m_fonts;
  int m_fontindex = -1;
  XftColor m_xftcolor;
};

namespace {
  /**
   * Configure injection module
   */
  template <typename T = unique_ptr<fontmanager>>
  di::injector<T> configure_fontmanager() {
    return di::make_injector(configure_connection(), configure_logger());
  }
}

LEMONBUDDY_NS_END
