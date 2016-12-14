#pragma once

#include "common.hpp"
#include "components/screen.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "events/signal_emitter.hpp"
#include "events/signal_fwd.hpp"
#include "utils/concurrency.hpp"
#include "utils/throttle.hpp"
#include "x11/connection.hpp"
#include "x11/events.hpp"
#include "x11/tray_manager.hpp"
#include "x11/types.hpp"
#include "x11/window.hpp"

POLYBAR_NS

// fwd
class screen;
class tray_manager;
class logger;
class renderer;

class bar : public xpp::event::sink<evt::button_press, evt::expose, evt::property_notify> {
 public:
  using make_type = unique_ptr<bar>;
  static make_type make();

  explicit bar(connection& conn, signal_emitter& emitter, const config& config, const logger& logger,
      unique_ptr<screen> screen, unique_ptr<tray_manager> tray_manager);

  ~bar();

  const bar_settings settings() const;

  void parse(const string& data, bool force = false);

 protected:
  void restack_window();
  void reconfigure_pos();
  void reconfigure_struts();
  void reconfigure_wm_hints();
  void broadcast_visibility();

  void handle(const evt::button_press& evt);
  void handle(const evt::expose& evt);
  void handle(const evt::property_notify& evt);

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const config& m_conf;
  const logger& m_log;
  unique_ptr<screen> m_screen;
  unique_ptr<tray_manager> m_tray;
  unique_ptr<renderer> m_renderer;

  bar_settings m_opts;

  string m_lastinput;

  std::mutex m_mutex;

  event_timer m_buttonpress{0L, 5L};
};

POLYBAR_NS_END
