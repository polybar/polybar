#pragma once

#include <cairo/cairo.h>
#include <bitset>

#include "cairo/fwd.hpp"
#include "common.hpp"
#include "components/types.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "x11/extensions/fwd.hpp"
#include "x11/types.hpp"

POLYBAR_NS

// fwd {{{
class connection;
class config;
class logger;
// }}}

using std::map;

class renderer
    : public signal_receiver<SIGN_PRIORITY_RENDERER, signals::parser::change_background,
          signals::parser::change_foreground, signals::parser::change_underline, signals::parser::change_overline,
          signals::parser::change_font, signals::parser::change_alignment, signals::parser::offset_pixel,
          signals::parser::attribute_set, signals::parser::attribute_unset, signals::parser::attribute_toggle,
          signals::parser::action_begin, signals::parser::action_end, signals::parser::text> {
 public:
  using make_type = unique_ptr<renderer>;
  static make_type make(const bar_settings& bar);

  explicit renderer(
      connection& conn, signal_emitter& emitter, const config&, const logger& logger, const bar_settings& bar);
  ~renderer();

  xcb_window_t window() const;
  const vector<action_block> actions() const;

  void begin();
  void end();
  void flush();

  void reserve_space(edge side, uint16_t w);
  void fill_background();
  void fill_overline(double x, double w);
  void fill_underline(double x, double w);
  void fill_borders();
  void draw_text(const string& contents);

 protected:
  void adjust_clickable_areas(double width);
  void highlight_clickable_areas();

  bool on(const signals::parser::change_background& evt);
  bool on(const signals::parser::change_foreground& evt);
  bool on(const signals::parser::change_underline& evt);
  bool on(const signals::parser::change_overline& evt);
  bool on(const signals::parser::change_font& evt);
  bool on(const signals::parser::change_alignment& evt);
  bool on(const signals::parser::offset_pixel& evt);
  bool on(const signals::parser::attribute_set& evt);
  bool on(const signals::parser::attribute_unset& evt);
  bool on(const signals::parser::attribute_toggle& evt);
  bool on(const signals::parser::action_begin& evt);
  bool on(const signals::parser::action_end& evt);
  bool on(const signals::parser::text& evt);

 protected:
  struct reserve_area {
    edge side{edge::NONE};
    uint16_t size{0U};
  };

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const config& m_conf;
  const logger& m_log;
  const bar_settings& m_bar;

  xcb_rectangle_t m_rect{0, 0, 0U, 0U};
  reserve_area m_cleararea{};

  uint8_t m_depth{32};
  xcb_window_t m_window;
  xcb_colormap_t m_colormap;
  xcb_visualtype_t* m_visual;
  xcb_gcontext_t m_gcontext;
  xcb_pixmap_t m_pixmap;

  vector<action_block> m_actions;

  // bool m_autosize{false};

  unique_ptr<cairo::context> m_context;
  unique_ptr<cairo::surface> m_surface;

  cairo_operator_t m_compositing_background;
  cairo_operator_t m_compositing_foreground;
  cairo_operator_t m_compositing_overline;
  cairo_operator_t m_compositing_underline;
  cairo_operator_t m_compositing_borders;

  alignment m_alignment{alignment::NONE};
  std::bitset<2> m_attributes;

  uint8_t m_fontindex;

  uint32_t m_color_background;
  uint32_t m_color_foreground;
  uint32_t m_color_overline;
  uint32_t m_color_underline;

  double m_x{0.0};
  double m_y{0.0};
};

POLYBAR_NS_END
