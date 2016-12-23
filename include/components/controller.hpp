#pragma once

#include <moodycamel/blockingconcurrentqueue.h>

#include "common.hpp"
#include "config.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "events/types.hpp"
#include "x11/types.hpp"

POLYBAR_NS

// fwd decl {{{

enum class alignment : uint8_t;
class bar;
class command;
class config;
class connection;
class inotify_watch;
class ipc;
class logger;
class signal_emitter;

namespace modules {
  struct module_interface;
}

using module_t = unique_ptr<modules::module_interface>;
using modulemap_t = std::map<alignment, vector<module_t>>;

// }}}

namespace chrono = std::chrono;
using namespace std::chrono_literals;

namespace sig_ev = signals::eventqueue;
namespace sig_ui = signals::ui;
namespace sig_ipc = signals::ipc;

class controller : public signal_receiver<SIGN_PRIORITY_CONTROLLER, sig_ev::process_broadcast, sig_ev::process_update,
                       sig_ev::process_input, sig_ev::process_quit, sig_ev::process_check, sig_ipc::process_action,
                       sig_ipc::process_command, sig_ipc::process_hook, sig_ui::button_press> {
 public:
  using make_type = unique_ptr<controller>;
  static make_type make(unique_ptr<ipc>&& ipc, unique_ptr<inotify_watch>&& config_watch);

  explicit controller(connection&, signal_emitter&, const logger&, const config&, unique_ptr<bar>&&, unique_ptr<ipc>&&,
      unique_ptr<inotify_watch>&&);
  ~controller();

  bool run(bool writeback = false);

  bool enqueue(event&& evt);
  bool enqueue(string&& input_data);

 protected:
  void read_events();
  void process_eventqueue();
  void process_inputdata();

  bool on(const sig_ev::process_broadcast& evt);
  bool on(const sig_ev::process_update& evt);
  bool on(const sig_ev::process_input& evt);
  bool on(const sig_ev::process_quit& evt);
  bool on(const sig_ev::process_check& evt);
  bool on(const sig_ui::button_press& evt);
  bool on(const sig_ipc::process_action& evt);
  bool on(const sig_ipc::process_command& evt);
  bool on(const sig_ipc::process_hook& evt);

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;
  const config& m_conf;
  unique_ptr<bar> m_bar;
  unique_ptr<ipc> m_ipc;
  unique_ptr<inotify_watch> m_confwatch;
  unique_ptr<command> m_command;

  unique_ptr<file_descriptor> m_fdevent_rd;
  unique_ptr<file_descriptor> m_fdevent_wr;

  /**
   * @brief Controls weather the output gets printed to stdout
   */
  bool m_writeback{false};

  /**
   * @brief Internal event queue
   */
  using queue_t = moodycamel::BlockingConcurrentQueue<event>;
  queue_t m_queue;

  /**
   * @brief Loaded modules
   */
  modulemap_t m_modules;

  /**
   * @brief Module input handlers
   */
  vector<signal_receiver_interface*> m_inputhandlers;

  /**
   * @brief Maximum number of subsequent events to swallow
   */
  size_t m_swallow_limit{5U};

  /**
   * @brief Time to wait for subsequent events
   */
  chrono::milliseconds m_swallow_update{10ms};

  /**
   * @brief Time to throttle input events
   */
  chrono::milliseconds m_swallow_input{30ms};

  /**
   * @brief Time of last handled input event
   */
  chrono::time_point<chrono::system_clock, chrono::milliseconds> m_lastinput;

  /**
   * @brief Input data
   */
  string m_inputdata;
};

POLYBAR_NS_END
