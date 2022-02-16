#pragma once

#include <cstdlib>

#include "common.hpp"
#include "components/eventloop.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "settings.hpp"
#include "tags/action_context.hpp"
#include "utils/math.hpp"
#include "x11/types.hpp"
#include "x11/window.hpp"

POLYBAR_NS

// fwd {{{
class config;
class connection;
class logger;
class renderer;
class screen;
class tray_manager;

namespace tags {
  class dispatch;
}
// }}}

class bar : public xpp::event::sink<evt::button_press, evt::expose, evt::property_notify, evt::enter_notify,
                evt::leave_notify, evt::motion_notify, evt::destroy_notify, evt::client_message, evt::configure_notify>,
            public signal_receiver<SIGN_PRIORITY_BAR, signals::ui::dim_window
#if WITH_XCURSOR
                ,
                signals::ui::cursor_change
#endif
                > {
 public:
  using make_type = unique_ptr<bar>;
  static make_type make(eventloop::loop&, bool only_initialize_values = false);

  explicit bar(connection&, signal_emitter&, const config&, const logger&, eventloop::loop&, unique_ptr<screen>&&,
      unique_ptr<tray_manager>&&, unique_ptr<tags::dispatch>&&, unique_ptr<tags::action_context>&&,
      bool only_initialize_values);
  ~bar();

  const bar_settings settings() const;

  void start();

  void parse(string&& data, bool force = false);

  void hide();
  void show();
  void toggle();

 protected:
  void restack_window();
  void reconfigure_window();
  void reconfigure_geom();
  void reconfigure_pos();
  void reconfigure_struts();
  void reconfigure_wm_hints();
  void broadcast_visibility();

  void trigger_click(mousebtn btn, int pos);

  void handle(const evt::client_message& evt) override;
  void handle(const evt::destroy_notify& evt) override;
  void handle(const evt::enter_notify& evt) override;
  void handle(const evt::leave_notify& evt) override;
  void handle(const evt::motion_notify& evt) override;
  void handle(const evt::button_press& evt) override;
  void handle(const evt::expose& evt) override;
  void handle(const evt::property_notify& evt) override;
  void handle(const evt::configure_notify& evt) override;

  bool on(const signals::ui::dim_window&) override;
#if WITH_XCURSOR
  bool on(const signals::ui::cursor_change&) override;
#endif

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const config& m_conf;
  const logger& m_log;
  eventloop::loop& m_loop;
  unique_ptr<screen> m_screen;
  unique_ptr<tray_manager> m_tray;
  unique_ptr<renderer> m_renderer;
  unique_ptr<tags::dispatch> m_dispatch;
  unique_ptr<tags::action_context> m_action_ctxt;

  bar_settings m_opts{};

  string m_lastinput{};
  bool m_dblclicks{false};

#if WITH_XCURSOR
  int m_motion_pos{0};
#endif

  eventloop::TimerHandle& m_leftclick_timer{m_loop.handle<eventloop::TimerHandle>()};
  eventloop::TimerHandle& m_middleclick_timer{m_loop.handle<eventloop::TimerHandle>()};
  eventloop::TimerHandle& m_rightclick_timer{m_loop.handle<eventloop::TimerHandle>()};
  eventloop::TimerHandle& m_dim_timer{m_loop.handle<eventloop::TimerHandle>()};

  bool m_visible{true};
};

POLYBAR_NS_END
