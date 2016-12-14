#pragma once

#include <X11/Xft/Xft.h>
#include <xcb/xcbext.h>

#include "common.hpp"
#include "x11/color.hpp"
#include "x11/types.hpp"

POLYBAR_NS

// fwd
class connection;
class logger;

struct fonttype {
  explicit fonttype() = default;
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
};

struct fonttype_deleter {
  void operator()(fonttype* f);
};

using fonttype_pointer = unique_ptr<fonttype, fonttype_deleter>;

class font_manager {
 public:
  using make_type = unique_ptr<font_manager>;
  static make_type make();

  explicit font_manager(connection& conn, const logger& logger, shared_ptr<Display>&& dsp, shared_ptr<Visual>&& vis);
  ~font_manager();

  bool load(const string& name, int8_t fontindex = DEFAULT_FONT_INDEX, int8_t offset_y = 0);

  void set_preferred_font(int8_t index);

  fonttype_pointer& match_char(uint16_t chr);
  uint8_t char_width(fonttype_pointer& font, uint16_t chr);

  XftColor* xftcolor();
  XftDraw* xftdraw();

  void create_xftdraw(xcb_pixmap_t pm);
  void destroy_xftdraw();

  void allocate_color(uint32_t color);
  void allocate_color(XRenderColor color);

  void set_gcontext_font(xcb_gcontext_t gc, xcb_font_t font);

 protected:
  bool open_xcb_font(fonttype_pointer& fontptr, string fontname);
  bool has_glyph(fonttype_pointer& font, uint16_t chr);

 private:
  connection& m_connection;
  const logger& m_logger;

  shared_ptr<Display> m_display{nullptr};
  shared_ptr<Visual> m_visual{nullptr};
  Colormap m_colormap{};

  std::map<uint8_t, fonttype_pointer> m_fonts;
  int8_t m_fontindex{DEFAULT_FONT_INDEX};

  XftColor* m_xftcolor{nullptr};
  XftDraw* m_xftdraw{nullptr};
};

POLYBAR_NS_END
