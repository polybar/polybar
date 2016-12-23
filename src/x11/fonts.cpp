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
    XftFontClose(&*xlib::get_display(), font->xft);
  }
  if (font->ptr != XCB_NONE) {
    xcb_close_font(&*xutils::get_connection(), font->ptr);
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

font_manager::font_manager(connection& conn, const logger& logger, Display* dsp, Visual* vis, Colormap&& cm)
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
      XftColorFree(m_display, m_visual, m_colormap, &m_xftcolor);
    }
    XFreeColormap(m_display, m_colormap);
  }
}

void font_manager::set_visual(Visual* v) {
  m_visual = v;
}

void font_manager::cleanup() {
  if (m_xftdraw != nullptr) {
    XftDrawDestroy(m_xftdraw);
    m_xftdraw = nullptr;
  }
}

bool font_manager::load(const string& name, uint8_t fontindex, int8_t offset_y) {
  if (fontindex > 0 && m_fonts.find(fontindex) != m_fonts.end()) {
    m_logger.warn("A font with index '%i' has already been loaded, skip...", fontindex);
    return false;
  } else if (fontindex == 0) {
    fontindex = m_fonts.size();
    m_logger.trace("font_manager: Assign font '%s' to index '%u'", name, fontindex);
  } else {
    m_logger.trace("font_manager: Add font '%s' to index '%u'", name, fontindex);
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
      (font->xft = XftFontOpenName(m_display, m_connection.default_screen(), name.c_str())) != nullptr) {
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

void font_manager::fontindex(uint8_t index) {
  if ((m_fontindex = index) > 0) {
    for (auto&& font : m_fonts) {
      if (font.first == index) {
        m_fontindex = index;
        break;
      }
    }
  }
}

shared_ptr<font_ref> font_manager::match_char(const uint16_t chr) {
  if (!m_fonts.empty()) {
    if (m_fontindex > 0 && static_cast<size_t>(m_fontindex) <= m_fonts.size()) {
      auto iter = m_fonts.find(m_fontindex);
      if (iter != m_fonts.end() && iter->second) {
        if (has_glyph_xft(iter->second, chr)) {
          return iter->second;
        } else if (has_glyph_xcb(iter->second, chr)) {
          return iter->second;
        }
      }
    }
    for (auto&& font : m_fonts) {
      if (font.second && has_glyph_xft(font.second, chr)) {
        return font.second;
      } else if (font.second && has_glyph_xcb(font.second, chr)) {
        return font.second;
      }
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
    m_xftdraw = XftDrawCreate(m_display, pm, m_visual, m_colormap);
  }
  if (font->xft != nullptr) {
    XftDrawString16(m_xftdraw, &m_xftcolor, font->xft, x, y, chars, num_chars);
  } else if (font->ptr != XCB_NONE) {
    vector<uint16_t> ucs(num_chars);
    for (size_t i = 0; i < num_chars; i++) {
      ucs[i] = (chars[i] >> 8) | (chars[i] << 8);
    }
    xcb_poly_text_16(pm, gc, x, y, num_chars, ucs.data());
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
    XftColorFree(m_display, m_visual, m_colormap, &m_xftcolor);
  }

  if (!(m_xftcolor_allocated = XftColorAllocValue(m_display, m_visual, m_colormap, &color, &m_xftcolor))) {
    m_logger.err("Failed to allocate color");
  }
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
  FT_UInt glyph{XftCharIndex(m_display, font->xft, static_cast<FcChar32>(chr))};

  XftFontLoadGlyphs(m_display, font->xft, FcFalse, &glyph, 1);
  XftGlyphExtents(m_display, font->xft, &glyph, 1, &extents);
  XftFontUnloadGlyphs(m_display, font->xft, &glyph, 1);

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
  } else if (XftCharExists(m_display, font->xft, static_cast<FcChar32>(chr)) == FcFalse) {
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

void font_manager::xcb_poly_text_16(
    xcb_drawable_t d, xcb_gcontext_t gc, int16_t x, int16_t y, uint8_t len, uint16_t* str) {
  static const xcb_protocol_request_t xcb_req = {5, nullptr, XCB_POLY_TEXT_16, 1};
  xcb_poly_text_16_request_t req{XCB_POLY_TEXT_16, 0, len, d, gc, x, y};
  uint8_t xcb_lendelta[2]{len, 0};
  struct iovec xcb_parts[7]{};
  xcb_parts[2].iov_base = reinterpret_cast<char*>(&req);
  xcb_parts[2].iov_len = sizeof(req);
  xcb_parts[3].iov_base = nullptr;
  xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
  xcb_parts[4].iov_base = xcb_lendelta;
  xcb_parts[4].iov_len = sizeof(xcb_lendelta);
  xcb_parts[5].iov_base = reinterpret_cast<char*>(str);
  xcb_parts[5].iov_len = len * sizeof(int16_t);
  xcb_parts[6].iov_base = nullptr;
  xcb_parts[6].iov_len = -(xcb_parts[4].iov_len + xcb_parts[5].iov_len) & 3;
  xcb_send_request(m_connection, 0, xcb_parts + 2, &xcb_req);
}

POLYBAR_NS_END
