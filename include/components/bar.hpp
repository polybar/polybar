#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "components/types.hpp"
#include "utils/concurrency.hpp"
#include "utils/throttle.hpp"
#include "x11/connection.hpp"
#include "x11/events.hpp"
#include "x11/types.hpp"
#include "x11/window.hpp"

POLYBAR_NS

// fwd
class tray_manager;
class logger;
class renderer;

class bar : public xpp::event::sink<evt::button_press, evt::expose, evt::property_notify> {
 public:
  explicit bar(connection& conn, const config& config, const logger& logger,
      unique_ptr<tray_manager> tray_manager);
  ~bar();

  void bootstrap(bool nodraw = false);
  void bootstrap_tray();
  void activate_tray();

  const bar_settings settings() const;

  void parse(string data, bool force = false);

 protected:
  void refresh_window();
  void create_monitor();
  void configure_geom();
  void restack_window();
  void map_window();
  void set_wmhints();

  int get_centerx();
  int get_innerwidth();

  void handle(const evt::button_press& evt);
  void handle(const evt::expose& evt);
  void handle(const evt::property_notify& evt);

  void on_alignment_change(const alignment align);
  void on_attribute_set(const attribute attr);
  void on_attribute_unset(const attribute attr);
  void on_attribute_toggle(const attribute attr);
  void on_action_block_open(const mousebtn btn, string cmd);
  void on_action_block_close(const mousebtn btn);
  void on_color_change(const gc gc_, uint32_t color);
  void on_font_change(int index);
  void on_pixel_offset(int px);
  void on_tray_report(uint16_t slots);

 private:
  connection& m_connection;
  const config& m_conf;
  const logger& m_log;
  unique_ptr<tray_manager> m_tray;
  unique_ptr<renderer> m_renderer;

  bar_settings m_opts;
  xcb_window_t m_window;
  xcb_screen_t* m_screen;
  size m_screensize{};
  bool m_sinkattached{false};
  string m_lastinput;

  alignment m_trayalign{alignment::NONE};
  uint8_t m_trayclients{0};

  std::mutex m_mutex;
};

di::injector<unique_ptr<bar>> configure_bar();

POLYBAR_NS_END
