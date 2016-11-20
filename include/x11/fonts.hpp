#pragma once

#include <X11/Xft/Xft.h>
#include <xcb/xcbext.h>

#include "common.hpp"
#include "components/logger.hpp"
#include "x11/color.hpp"
#include "x11/types.hpp"

POLYBAR_NS

// fwd
class connection;

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
  void operator()(fonttype* f);
};

using font_t = unique_ptr<fonttype, fonttype_deleter>;

class font_manager {
 public:
  explicit font_manager(connection& conn, const logger& logger);
  ~font_manager();

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

di::injector<unique_ptr<font_manager>> configure_font_manager();

POLYBAR_NS_END
