#pragma once

#include <cairo/cairo.h>

#include <bitset>
#include <memory>

#include "cairo/fwd.hpp"
#include "common.hpp"
#include "components/renderer_interface.hpp"
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
class background_manager;
class bg_slice;
// }}}

using std::map;

struct alignment_block {
  cairo_pattern_t* pattern;
  double x;
  double y;
};

class renderer : public renderer_interface,
                 public signal_receiver<SIGN_PRIORITY_RENDERER, signals::ui::request_snapshot,
                     signals::parser::change_alignment, signals::parser::action_begin, signals::parser::action_end> {
 public:
  using make_type = unique_ptr<renderer>;
  static make_type make(const bar_settings& bar);

  explicit renderer(connection& conn, signal_emitter& sig, const config&, const logger& logger, const bar_settings& bar,
      background_manager& background_manager);
  ~renderer();

  xcb_window_t window() const;
  const vector<action_block> actions() const;

  void begin(xcb_rectangle_t rect);
  void end();
  void flush();

#if 0
  void reserve_space(edge side, unsigned int w);
#endif
  void fill_background();
  void fill_overline(rgba color, double x, double w);
  void fill_underline(rgba color, double x, double w);
  void fill_borders();

  void render_offset(const tags::context& ctxt, int pixels) override;
  void render_text(const tags::context& ctxt, const string&&) override;

 protected:
  double block_x(alignment a) const;
  double block_y(alignment a) const;
  double block_w(alignment a) const;
  double block_h(alignment a) const;

  void flush(alignment a);
  void highlight_clickable_areas();

  bool on(const signals::ui::request_snapshot& evt) override;
  bool on(const signals::parser::change_alignment& evt) override;
  bool on(const signals::parser::action_begin& evt) override;
  bool on(const signals::parser::action_end& evt) override;

 protected:
  struct reserve_area {
    edge side{edge::NONE};
    unsigned int size{0U};
  };

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const config& m_conf;
  const logger& m_log;
  const bar_settings& m_bar;
  std::shared_ptr<bg_slice> m_background;

  int m_depth{32};
  xcb_window_t m_window;
  xcb_colormap_t m_colormap;
  xcb_visualtype_t* m_visual;
  xcb_gcontext_t m_gcontext;
  xcb_pixmap_t m_pixmap;

  xcb_rectangle_t m_rect{0, 0, 0U, 0U};
  reserve_area m_cleararea{};

  // bool m_autosize{false};

  unique_ptr<cairo::context> m_context;
  unique_ptr<cairo::xcb_surface> m_surface;
  map<alignment, alignment_block> m_blocks;
  cairo_pattern_t* m_cornermask{};

  cairo_operator_t m_comp_bg{CAIRO_OPERATOR_SOURCE};
  cairo_operator_t m_comp_fg{CAIRO_OPERATOR_OVER};
  cairo_operator_t m_comp_ol{CAIRO_OPERATOR_OVER};
  cairo_operator_t m_comp_ul{CAIRO_OPERATOR_OVER};
  cairo_operator_t m_comp_border{CAIRO_OPERATOR_OVER};
  bool m_pseudo_transparency{false};

  alignment m_align;
  vector<action_block> m_actions;

  bool m_fixedcenter;
  string m_snapshot_dst;
};

POLYBAR_NS_END
