#pragma once

#include "common.hpp"
#include "components/types.hpp"
#include "events/signal_emitter.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "x11/fonts.hpp"
#include "x11/types.hpp"

POLYBAR_NS

// fwd
class connection;
class font_manager;
class logger;

using namespace signals::parser;
using std::map;

class renderer
    : public signal_receiver<SIGN_PRIORITY_RENDERER, change_background, change_foreground, change_underline,
          change_overline, change_font, change_alignment, offset_pixel, attribute_set, attribute_unset,
          attribute_toggle, action_begin, action_end, write_text_ascii, write_text_unicode, write_text_string> {
 public:
  enum class gc : uint8_t { BG, FG, OL, UL, BT, BB, BL, BR };

  using make_type = unique_ptr<renderer>;
  static make_type make(const bar_settings& bar, vector<string>&& fonts);

  explicit renderer(connection& conn, signal_emitter& emitter, const logger& logger,
      unique_ptr<font_manager> font_manager, const bar_settings& bar, const vector<string>& fonts);
  ~renderer();

  xcb_window_t window() const;

  void begin();
  void end();
  void flush(bool clear);

  void reserve_space(edge side, uint16_t w);

  void set_background(const uint32_t color);
  void set_foreground(const uint32_t color);
  void set_underline(const uint32_t color);
  void set_overline(const uint32_t color);
  void set_fontindex(const int8_t font);
  void set_alignment(const alignment align);
  void set_attribute(const attribute attr, const bool state);
  void toggle_attribute(const attribute attr);
  bool check_attribute(const attribute attr);

  void fill_background();
  void fill_overline(int16_t x, uint16_t w);
  void fill_underline(int16_t x, uint16_t w);
  void fill_shift(const int16_t px);

  void draw_character(const uint16_t character);
  void draw_textstring(const char* text, const size_t len);

  void begin_action(const mousebtn btn, const string& cmd);
  void end_action(const mousebtn btn);
  const vector<action_block> get_actions();

 protected:
  int16_t shift_content(int16_t x, const int16_t shift_x);
  int16_t shift_content(const int16_t shift_x);

  bool on(const change_background& evt);
  bool on(const change_foreground& evt);
  bool on(const change_underline& evt);
  bool on(const change_overline& evt);
  bool on(const change_font& evt);
  bool on(const change_alignment& evt);
  bool on(const offset_pixel& evt);
  bool on(const attribute_set& evt);
  bool on(const attribute_unset& evt);
  bool on(const attribute_toggle& evt);
  bool on(const action_begin& evt);
  bool on(const action_end& evt);
  bool on(const write_text_ascii& evt);
  bool on(const write_text_unicode& evt);
  bool on(const write_text_string& evt);

#ifdef DEBUG_HINTS
  vector<xcb_window_t> m_debughints;
  void debug_hints();
#endif

 protected:
  struct reserve_area {
    edge side{edge::NONE};
    uint16_t size{0U};
  };

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;
  unique_ptr<font_manager> m_fontmanager;

  const bar_settings& m_bar;

  xcb_rectangle_t m_rect{0, 0, 0U, 0U};
  xcb_rectangle_t m_cleared{0, 0, 0U, 0U};
  reserve_area m_cleararea{};

  xcb_window_t m_window;
  xcb_colormap_t m_colormap;
  xcb_visualtype_t* m_visual;
  // xcb_gcontext_t m_gcontext;
  xcb_pixmap_t m_pixmap;

  map<gc, xcb_gcontext_t> m_gcontexts;
  map<alignment, xcb_pixmap_t> m_pixmaps;
  vector<action_block> m_actions;

  // bool m_autosize{false};
  uint16_t m_currentx{0U};
  alignment m_alignment{alignment::NONE};
  map<gc, uint32_t> m_colors;
  uint8_t m_attributes{0U};
  int8_t m_fontindex{DEFAULT_FONT_INDEX};

  xcb_font_t m_gcfont{XCB_NONE};
};

POLYBAR_NS_END
