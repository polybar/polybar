#pragma once

#include <atomic>
#include <mutex>

#include "common.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "settings.hpp"
#include "x11/events.hpp"
#include "x11/types.hpp"
#include "x11/window.hpp"

POLYBAR_NS

// fwd {{{
class config;
class connection;
class logger;
class parser;
class renderer;
class screen;
class taskqueue;
class tray_manager;
// }}}

class bar : public xpp::event::sink<evt::button_press, evt::expose, evt::property_notify, evt::enter_notify,
                evt::leave_notify, evt::destroy_notify, evt::client_message>,
            public signal_receiver<SIGN_PRIORITY_BAR, signals::eventqueue::start, signals::ui::tick,
                signals::ui::shade_window, signals::ui::unshade_window, signals::ui::dim_window> {
 public:
  using make_type = unique_ptr<bar>;
  static make_type make(bool only_initialize_values = false);

  explicit bar(connection&, signal_emitter&, const config&, const logger&, unique_ptr<screen>&&,
      unique_ptr<tray_manager>&&, unique_ptr<parser>&&, unique_ptr<taskqueue>&&, bool only_initialize_values);
  ~bar();

  const bar_settings settings() const;

  void parse(string&& data, bool force = false);

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

  bool on(const signals::eventqueue::start&);
  bool on(const signals::ui::unshade_window&);
  bool on(const signals::ui::shade_window&);
  bool on(const signals::ui::tick&);
  bool on(const signals::ui::dim_window&);

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const config& m_conf;
  const logger& m_log;
  unique_ptr<screen> m_screen;
  unique_ptr<tray_manager> m_tray{};
  unique_ptr<renderer> m_renderer{};
  unique_ptr<parser> m_parser{};
  unique_ptr<taskqueue> m_taskqueue;

  bar_settings m_opts{};

  string m_lastinput{};
  std::mutex m_mutex{};
  std::atomic<bool> m_dblclicks{false};

  mousebtn m_buttonpress_btn{mousebtn::NONE};
  int16_t m_buttonpress_pos{0};

  event_timer m_buttonpress{0L, 5L};
  event_timer m_doubleclick{0L, 150L};

  double m_anim_step{0};
};

POLYBAR_NS_END
