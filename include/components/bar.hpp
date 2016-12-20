#pragma once

#include <mutex>

#include "common.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "events/signal_fwd.hpp"
#include "x11/events.hpp"
#include "x11/types.hpp"
#include "x11/window.hpp"

POLYBAR_NS

// fwd
class connection;
class logger;
class parser;
class renderer;
class screen;
class signal_emitter;
class tray_manager;

class bar : public xpp::event::sink<evt::button_press, evt::expose, evt::property_notify, evt::enter_notify,
                evt::leave_notify, evt::destroy_notify, evt::client_message> {
 public:
  using make_type = unique_ptr<bar>;
  static make_type make();

  explicit bar(connection& conn, signal_emitter& emitter, const config& config, const logger& logger,
      unique_ptr<screen>&& screen, unique_ptr<tray_manager>&& tray_manager, unique_ptr<parser>&& parser);
  virtual ~bar();

  void parse(string&& data) const;
  void parse(const string& data, bool force = false);

  const bar_settings settings() const;

 protected:
  void restack_window();
  void reconfigure_pos();
  void reconfigure_struts();
  void reconfigure_wm_hints();
  void broadcast_visibility();

  void handle(const evt::client_message& evt);
  void handle(const evt::destroy_notify& evt);
  void handle(const evt::enter_notify& evt);
  void handle(const evt::leave_notify& evt);
  void handle(const evt::button_press& evt);
  void handle(const evt::expose& evt);
  void handle(const evt::property_notify& evt);

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const config& m_conf;
  const logger& m_log;
  unique_ptr<screen> m_screen;
  unique_ptr<tray_manager> m_tray{};
  unique_ptr<renderer> m_renderer{};
  unique_ptr<parser> m_parser{};

  bar_settings m_opts{};

  string m_lastinput{};

  std::mutex m_mutex{};

  event_timer m_buttonpress{0L, 5L};
  event_timer m_doubleclick{0L, 250L};
};

POLYBAR_NS_END
