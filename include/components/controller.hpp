#pragma once

#include <csignal>

#include "common.hpp"
#include "components/bar.hpp"
#include "components/config.hpp"
#include "components/eventloop.hpp"
#include "components/ipc.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "config.hpp"
#include "events/signal_emitter.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "utils/command.hpp"
#include "utils/inotify.hpp"
#include "x11/connection.hpp"
#include "x11/types.hpp"

POLYBAR_NS

// fwd decl {{{

class bar;
struct bar_settings;

// }}}

using watch_t = inotify_util::watch_t;

namespace sig_ev = signals::eventloop;
namespace sig_ui = signals::ui;
namespace sig_ipc = signals::ipc;

class controller : public signal_receiver<SIGN_PRIORITY_CONTROLLER, sig_ev::process_update, sig_ev::process_input,
                       sig_ev::process_quit, sig_ui::button_press, sig_ipc::process_action, sig_ipc::process_command,
                       sig_ipc::process_hook> {
 public:
  explicit controller(connection& conn, signal_emitter& emitter, const logger& logger, const config& config,
      unique_ptr<eventloop> eventloop, unique_ptr<bar> bar, unique_ptr<ipc> ipc, watch_t confwatch, bool writeback);
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

  sigset_t m_waitmask;
  vector<thread> m_threads;

  watch_t m_confwatch;
  command_t m_command;

  bool m_writeback{false};
};

namespace {
  inline unique_ptr<controller> make_controller(watch_t&& confwatch, bool enableipc = false, bool writeback = false) {
    unique_ptr<ipc> ipc;

    if (enableipc) {
      ipc = make_ipc();
    }

    // clang-format off
    return factory_util::unique<controller>(
        make_connection(),
        make_signal_emitter(),
        make_logger(),
        make_confreader(),
        make_eventloop(),
        make_bar(),
        move(ipc),
        move(confwatch),
        writeback);
    // clang-format on
  }
}

POLYBAR_NS_END
