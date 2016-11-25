#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "components/eventloop.hpp"
#include "components/ipc.hpp"
#include "components/logger.hpp"
#include "config.hpp"
#include "utils/command.hpp"
#include "utils/inotify.hpp"
#include "x11/connection.hpp"
#include "x11/types.hpp"

POLYBAR_NS

// fwd decl {{{

class bar;

// }}}

using watch_t = inotify_util::watch_t;

class controller {
 public:
  explicit controller(connection& conn, const logger& logger, const config& config, unique_ptr<eventloop> eventloop,
      unique_ptr<bar> bar, inotify_util::watch_t& confwatch)
      : m_connection(conn)
      , m_log(logger)
      , m_conf(config)
      , m_eventloop(forward<decltype(eventloop)>(eventloop))
      , m_bar(forward<decltype(bar)>(bar))
      , m_confwatch(confwatch) {}

  ~controller();

  void bootstrap(bool writeback = false, bool dump_wmname = false);

  void run();
  bool completed();

 protected:
  void install_sigmask();
  void uninstall_sigmask();

  void install_confwatch();
  void uninstall_confwatch();

  void wait_for_signal();
  void wait_for_xevent();

  void bootstrap_modules();

  void on_ipc_action(const ipc_action& message);
  void on_mouse_event(string input);
  void on_unrecognized_action(string input);
  void on_update();

 private:
  connection& m_connection;
  registry m_registry{m_connection};
  const logger& m_log;
  const config& m_conf;
  unique_ptr<eventloop> m_eventloop;
  unique_ptr<bar> m_bar;
  unique_ptr<ipc> m_ipc;

  stateflag m_running{false};
  stateflag m_reload{false};
  stateflag m_waiting{false};
  stateflag m_trayactivated{false};

  sigset_t m_waitmask;
  sigset_t m_ignmask;

  vector<thread> m_threads;

  inotify_util::watch_t& m_confwatch;
  command_util::command_t m_command;

  bool m_writeback{false};
};

di::injector<unique_ptr<controller>> configure_controller(watch_t& confwatch);

POLYBAR_NS_END
