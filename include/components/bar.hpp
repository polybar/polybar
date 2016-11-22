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
  void setup_monitor();
  void configure_geom();
  void restack_window();
  void reconfigure_window();

  void handle(const evt::button_press& evt);
  void handle(const evt::expose& evt);
  void handle(const evt::property_notify& evt);

 private:
  connection& m_connection;
  const config& m_conf;
  const logger& m_log;
  unique_ptr<tray_manager> m_tray;
  unique_ptr<renderer> m_renderer;

  xcb_screen_t* m_screen;
  xcb_window_t m_window;

  bar_settings m_opts;

  alignment m_trayalign{alignment::NONE};
  uint8_t m_trayclients{0};

  std::mutex m_mutex;

  string m_lastinput;
};

di::injector<unique_ptr<bar>> configure_bar();

POLYBAR_NS_END
