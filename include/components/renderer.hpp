#pragma once

#include "common.hpp"
#include "components/types.hpp"
#include "x11/types.hpp"

POLYBAR_NS

class connection;
class font_manager;
class logger;

class renderer {
 public:
  explicit renderer(connection& conn, const logger& logger, unique_ptr<font_manager> font_manager,
      const bar_settings& bar, const vector<string>& fonts);

  xcb_window_t window() const;

  void begin();
  void end();
  void redraw();

  void reserve_space(edge side, uint16_t w);

  void set_background(const gc gcontext, const uint32_t color);
  void set_foreground(const gc gcontext, const uint32_t color);
  void set_fontindex(const uint8_t font);
  void set_alignment(const alignment align);
  void set_attribute(const attribute attr, const bool state);

  void fill_background();
  void fill_border(const map<edge, border_settings>& borders, edge border);
  void fill_overline(int16_t x, uint16_t w);
  void fill_underline(int16_t x, uint16_t w);

  void draw_character(uint16_t character);
  void draw_textstring(const char* text, size_t len);

  int16_t shift_content(int16_t x, int16_t shift_x);
  int16_t shift_content(int16_t shift_x);

  void begin_action(const mousebtn btn, const string& cmd);
  void end_action(const mousebtn btn);
  const vector<action_block> get_actions();

 protected:
  void debughints();

 private:
  connection& m_connection;
  const logger& m_log;
  unique_ptr<font_manager> m_fontmanager;

  const bar_settings& m_bar;

  xcb_window_t m_window;
  xcb_colormap_t m_colormap;
  xcb_visualtype_t* m_visual;
  // xcb_gcontext_t m_gcontext;
  xcb_pixmap_t m_pixmap;

  map<gc, xcb_gcontext_t> m_gcontexts;
  map<alignment, xcb_pixmap_t> m_pixmaps;
  vector<action_block> m_actions;

  // bool m_autosize{false};
  int m_currentx{0};
  int m_attributes{0};
  alignment m_alignment{alignment::NONE};

  xcb_font_t m_gcfont{0};

  uint32_t m_background{0};
  uint32_t m_foreground{0};
  gc m_background_gc{gc::NONE};
  gc m_foreground_gc{gc::NONE};

  edge m_reserve_at{edge::NONE};
  uint16_t m_reserve;
};

di::injector<unique_ptr<renderer>> configure_renderer(const bar_settings& bar, const vector<string>& fonts);

POLYBAR_NS_END
