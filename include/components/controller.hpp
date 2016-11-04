#pragma once

#include "common.hpp"
#include "components/bar.hpp"
#include "components/config.hpp"
#include "components/eventloop.hpp"
#include "components/logger.hpp"
#include "components/signals.hpp"
#include "config.hpp"
#include "utils/command.hpp"
#include "utils/inotify.hpp"
#include "x11/connection.hpp"
#include "x11/types.hpp"

LEMONBUDDY_NS

class controller {
 public:
  explicit controller(connection& conn, const logger& logger, const config& config,
      unique_ptr<eventloop> eventloop, unique_ptr<bar> bar,
      inotify_util::watch_t& confwatch)
      : m_connection(conn)
      , m_log(logger)
      , m_conf(config)
      , m_eventloop(forward<decltype(eventloop)>(eventloop))
      , m_bar(forward<decltype(bar)>(bar))
      , m_confwatch(confwatch) {}

  ~controller();

  void bootstrap(bool writeback = false, bool dump_wmname = false);
  bool run();

 protected:
  void install_sigmask();
  void uninstall_sigmask();

  void install_confwatch();
  void uninstall_confwatch();

  void wait_for_signal();
  void wait_for_xevent();

  void bootstrap_modules();

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

  stateflag m_running{false};
  stateflag m_reload{false};
  stateflag m_waiting{false};
  stateflag m_trayactivated{false};

  sigset_t m_waitmask;
  sigset_t m_ignmask;

  vector<thread> m_threads;

  inotify_util::watch_t& m_confwatch;
  command_util::command_t m_command;

  bool m_writeback = false;
};

namespace {
  /**
   * Configure injection module
   */
  template <typename T = unique_ptr<controller>>
  di::injector<T> configure_controller(inotify_util::watch_t& confwatch) {
    // clang-format off
    return di::make_injector(
        di::bind<>().to(confwatch),
        configure_connection(),
        configure_logger(),
        configure_config(),
        configure_eventloop(),
        configure_bar());
    // clang-format on
  }
}

LEMONBUDDY_NS_END
