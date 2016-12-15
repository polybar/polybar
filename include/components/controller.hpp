#pragma once

#include <csignal>

#include "common.hpp"
#include "components/bar.hpp"
#include "components/ipc.hpp"
#include "components/types.hpp"
#include "config.hpp"
#include "events/signal_emitter.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "x11/types.hpp"

POLYBAR_NS

// fwd decl {{{

class bar;
class config;
class connection;
class eventloop;
class logger;
struct bar_settings;
namespace inotify_util {
  class inotify_watch;
  using watch_t = unique_ptr<inotify_watch>;
}
namespace command_util {
  class command;
}
using command = command_util::command;
using command_t = unique_ptr<command>;

// }}}

using watch_t = inotify_util::watch_t;

namespace sig_ev = signals::eventloop;
namespace sig_ui = signals::ui;
namespace sig_ipc = signals::ipc;

class controller : public signal_receiver<SIGN_PRIORITY_CONTROLLER, sig_ev::process_update, sig_ev::process_input,
                       sig_ev::process_quit, sig_ui::button_press, sig_ipc::process_action, sig_ipc::process_command,
                       sig_ipc::process_hook> {
 public:
  using make_type = unique_ptr<controller>;
  static make_type make(string&& path_confwatch, bool enable_ipc = false, bool writeback = false);

  explicit controller(connection& conn, signal_emitter& emitter, const logger& logger, const config& config,
      unique_ptr<eventloop>&& eventloop, unique_ptr<bar>&& bar, unique_ptr<ipc>&& ipc, watch_t&& confwatch, bool writeback);
  ~controller();

  void setup();
  bool run();

  const bar_settings opts() const;

 protected:
  void wait_for_signal();
  void wait_for_xevent();
  void wait_for_eventloop();
  void wait_for_configwatch();

  bool on(const sig_ev::process_update& evt);
  bool on(const sig_ev::process_input& evt);
  bool on(const sig_ev::process_quit& evt);
  bool on(const sig_ui::button_press& evt);
  bool on(const sig_ipc::process_action& evt);
  bool on(const sig_ipc::process_command& evt);
  bool on(const sig_ipc::process_hook& evt);

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;
  const config& m_conf;
  unique_ptr<eventloop> m_eventloop;
  unique_ptr<bar> m_bar;
  unique_ptr<ipc> m_ipc;

  stateflag m_running{false};
  stateflag m_reload{false};
  stateflag m_waiting{false};

  sigset_t m_waitmask{};
  vector<thread> m_threads;

  watch_t m_confwatch;
  command_t m_command;

  bool m_writeback{false};
};

POLYBAR_NS_END
