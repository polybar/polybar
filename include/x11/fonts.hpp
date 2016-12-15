#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H

#include <X11/Xft/Xft.h>
#include <xcb/xcbext.h>
#include <unordered_map>

#include "common.hpp"
#include "x11/color.hpp"
#include "x11/types.hpp"

POLYBAR_NS

using std::map;
using std::unordered_map;

// fwd
class connection;
class logger;

struct font_ref {
  explicit font_ref() = default;
  font_ref(const font_ref& o) = delete;
  font_ref& operator=(const font_ref& o) = delete;
  XftFont* xft{nullptr};
  xcb_font_t ptr{XCB_NONE};
  int offset_y{0};
  int ascent{0};
  int descent{0};
  int height{0};
  int width{0};
  uint16_t char_max{0};
  uint16_t char_min{0};
  vector<xcb_charinfo_t> width_lut{};
  unordered_map<uint16_t, wchar_t> glyph_widths{};

  static struct _deleter {
    void operator()(font_ref* font);
  } deleter;
};

class font_manager {
 public:
  using make_type = unique_ptr<font_manager>;
  static make_type make();

  explicit font_manager(
      connection& conn, const logger& logger, shared_ptr<Display>&& dsp, shared_ptr<Visual>&& vis, Colormap&& cm);
  ~font_manager();

  font_manager(const font_manager& o) = delete;
  font_manager& operator=(const font_manager& o) = delete;

  void cleanup();
  bool load(const string& name, int8_t fontindex = DEFAULT_FONT_INDEX, int8_t offset_y = 0);
  void set_preferred_font(int8_t index);
  shared_ptr<font_ref> match_char(const uint16_t chr);
  uint8_t glyph_width(const shared_ptr<font_ref>& font, const uint16_t chr);
  void drawtext(const shared_ptr<font_ref>& font, xcb_pixmap_t pm, xcb_gcontext_t gc, int16_t x, int16_t y,
      const uint16_t* chars, size_t num_chars);

  void allocate_color(uint32_t color);
  void allocate_color(XRenderColor color);

  void set_gcontext_font(const shared_ptr<font_ref>& font, xcb_gcontext_t, xcb_font_t*);

 protected:
  bool open_xcb_font(const shared_ptr<font_ref>& font, string fontname);

  uint8_t glyph_width_xft(const shared_ptr<font_ref>& font, const uint16_t chr);
  uint8_t glyph_width_xcb(const shared_ptr<font_ref>& font, const uint16_t chr);

  bool has_glyph_xft(const shared_ptr<font_ref>& font, const uint16_t chr);
  bool has_glyph_xcb(const shared_ptr<font_ref>& font, const uint16_t chr);

 private:
  connection& m_connection;
  const logger& m_logger;

  shared_ptr<Display> m_display;
  shared_ptr<Visual> m_visual;
  Colormap m_colormap;

  map<uint8_t, shared_ptr<font_ref>> m_fonts{};
  int8_t m_fontindex{DEFAULT_FONT_INDEX};

  XftDraw* m_xftdraw{nullptr};
  XftColor m_xftcolor{};
  bool m_xftcolor_allocated{false};
};

POLYBAR_NS_END
