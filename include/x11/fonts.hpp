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

  bool load(string name, int8_t fontindex = -1, int8_t offset_y = 0);

  void set_preferred_font(int8_t index);

  font_t& match_char(uint16_t chr);
  uint8_t char_width(font_t& font, uint16_t chr);

  XftColor xftcolor();
  XftDraw* xftdraw();
  XftDraw* create_xftdraw(xcb_pixmap_t pm, xcb_colormap_t cm);
  void destroy_xftdraw();

  void allocate_color(uint32_t color, bool initial_alloc = false);
  void allocate_color(XRenderColor color, bool initial_alloc = false);

  void set_gcontext_font(xcb_gcontext_t gc, xcb_font_t font);

 protected:
  bool open_xcb_font(font_t& fontptr, string fontname);
  bool has_glyph(font_t& font, uint16_t chr);

 private:
  connection& m_connection;
  const logger& m_logger;

  Display* m_display{nullptr};
  Visual* m_visual{nullptr};
  Colormap m_colormap{};

  map<uint8_t, font_t> m_fonts;
  int8_t m_fontindex{-1};

  XftColor m_xftcolor{};
  XftDraw* m_xftdraw{nullptr};
};

di::injector<unique_ptr<font_manager>> configure_font_manager();

POLYBAR_NS_END
