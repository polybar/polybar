#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "utils/threading.hpp"
#include "utils/throttle.hpp"
#include "x11/connection.hpp"
#include "x11/fontmanager.hpp"
#include "x11/tray.hpp"
#include "x11/types.hpp"
#include "x11/window.hpp"

POLYBAR_NS

// fwd
class tray_manager;

class bar : public xpp::event::sink<evt::button_press, evt::expose, evt::property_notify> {
 public:
  explicit bar(connection& conn, const config& config, const logger& logger, unique_ptr<fontmanager> fontmanager,
      unique_ptr<tray_manager> tray_manager)
      : m_connection(conn)
      , m_conf(config)
      , m_log(logger)
      , m_fontmanager(forward<decltype(fontmanager)>(fontmanager))
      , m_tray(forward<decltype(tray_manager)>(tray_manager)) {}

  ~bar();

  void bootstrap(bool nodraw = false);
  void bootstrap_tray();
  void activate_tray();

  const bar_settings settings() const;

  void parse(string data, bool force = false);

 protected:
  void flush();
  void refresh_window();
  void load_fonts();
  void configure_geom();
  void create_monitor();
  void create_window();
  void create_pixmap();
  void create_gcontexts();
  void restack_window();
  void map_window();
  void set_wmhints();

  int get_centerx();
  int get_innerwidth();

  void handle(const evt::button_press& evt);
  void handle(const evt::expose& evt);
  void handle(const evt::property_notify& evt);

  void on_alignment_change(alignment align);
  void on_attribute_set(attribute attr);
  void on_attribute_unset(attribute attr);
  void on_attribute_toggle(attribute attr);
  void on_action_block_open(mousebtn btn, string cmd);
  void on_action_block_close(mousebtn btn);
  void on_color_change(gc gc_, color color_);
  void on_font_change(int index);
  void on_pixel_offset(int px);
  void on_tray_report(uint16_t slots);

  void draw_background();
  void draw_border(border border_);
  void draw_lines(int x, int w);
  int draw_shift(int x, int chr_width);
  void draw_character(uint16_t character);
  void draw_textstring(const char* text, size_t len);

 private:
  connection& m_connection;
  const config& m_conf;
  const logger& m_log;
  unique_ptr<fontmanager> m_fontmanager;
  unique_ptr<tray_manager> m_tray;

  threading_util::spin_lock m_lock;
  throttle_util::throttle_t m_throttler;

  xcb_screen_t* m_screen;
  rect m_screensize{};

  xcb_visualtype_t* m_visual;

  window m_window{m_connection, m_connection.generate_id()};
  colormap m_colormap{m_connection, m_connection.generate_id()};
  pixmap m_pixmap{m_connection, m_connection.generate_id()};

  // xcb_gcontext_t m_root_gc{0};
  // graphics_util::root_pixmap m_rootpixmap;

  bar_settings m_opts;
  map<border, border_settings> m_borders;
  map<gc, gcontext> m_gcontexts;
  vector<action_block> m_actions;

  stateflag m_sinkattached{false};

  alignment m_traypos{alignment::NONE};
  uint16_t m_trayclients{0};

  string m_prevdata;
  int m_xpos{0};
  int m_attributes{0};

  xcb_font_t m_gcfont{0};
  XftDraw* m_xftdraw;
};

namespace {
  /**
   * Configure injection module
   */
  template <typename T = unique_ptr<bar>>
  di::injector<T> configure_bar() {
    // clang-format off
    return di::make_injector(
        configure_connection(),
        configure_config(),
        configure_logger(),
        configure_fontmanager(),
        configure_tray_manager());
    // clang-format on
  }
}

POLYBAR_NS_END
