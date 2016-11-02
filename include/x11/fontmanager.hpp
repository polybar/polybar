#pragma once

#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcbext.h>

#include "common.hpp"
#include "components/logger.hpp"
#include "x11/color.hpp"
#include "x11/connection.hpp"
#include "x11/types.hpp"
#include "x11/xlib.hpp"

LEMONBUDDY_NS

#define XFT_MAXCHARS (1 << 16)
extern array<char, XFT_MAXCHARS> xft_widths;
extern array<wchar_t, XFT_MAXCHARS> xft_chars;

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
  explicit fontmanager(connection& conn, const logger& logger);
  ~fontmanager();

  void set_preferred_font(int index);

  bool load(string name, int fontindex = -1, int offset_y = 0);

  font_t& match_char(uint16_t chr);

  int char_width(font_t& font, uint16_t chr);

  XftColor xftcolor();

  void allocate_color(XRenderColor color, bool initial_alloc = false);

  void set_gcontext_font(gcontext& gc, xcb_font_t font);

 protected:
  bool open_xcb_font(font_t& fontptr, string fontname);

  bool has_glyph(font_t& font, uint16_t chr);

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
