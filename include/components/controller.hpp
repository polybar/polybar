#pragma once

#include <mutex>
#include <queue>

#include "common.hpp"
#include "components/eventloop.hpp"
#include "components/types.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "settings.hpp"
#include "utils/actions.hpp"
#include "utils/file.hpp"
#include "x11/types.hpp"

POLYBAR_NS

using std::queue;

// fwd decl {{{

enum class alignment;
class bar;
class config;
class connection;
class inotify_watch;
class logger;
class signal_emitter;
namespace modules {
  struct module_interface;
} // namespace modules
using module_t = shared_ptr<modules::module_interface>;
using modulemap_t = std::map<alignment, vector<module_t>>;
// }}}

class controller : public signal_receiver<SIGN_PRIORITY_CONTROLLER, signals::eventqueue::exit_reload,
                       signals::eventqueue::notify_change, signals::eventqueue::notify_forcechange,
                       signals::eventqueue::check_state, signals::ipc::action, signals::ipc::command,
                       signals::ipc::hook, signals::ui::button_press, signals::ui::update_background> {
 public:
  using make_type = unique_ptr<controller>;
  static make_type make(bool has_ipc, eventloop::loop&, const config&);

  explicit controller(connection&, signal_emitter&, const logger&, const config&, bool has_ipc, eventloop::loop&);
  ~controller();

  bool run(bool writeback, string snapshot_dst, bool confwatch);

  void trigger_action(string&& input_data);
  void trigger_quit(bool reload);
  void trigger_update(bool force);

  void stop(bool reload);

  void signal_handler(int signum);

  void conn_cb();
  void create_config_watcher(const string& filename);
  void confwatch_handler(const char* fname);
  void notifier_handler();
  void screenshot_handler();

 protected:
  void trigger_notification();
  void start_modules();
  void read_events(bool confwatch);
  void process_inputdata(string&& cmd);
  bool process_update(bool force);

  void update_reload(bool reload);

  bool on(const signals::eventqueue::notify_change& evt) override;
  bool on(const signals::eventqueue::notify_forcechange& evt) override;
  bool on(const signals::eventqueue::exit_reload& evt) override;
  bool on(const signals::eventqueue::check_state& evt) override;
  bool on(const signals::ui::button_press& evt) override;
  bool on(const signals::ipc::action& evt) override;
  bool on(const signals::ipc::command& evt) override;
  bool on(const signals::ipc::hook& evt) override;
  bool on(const signals::ui::update_background& evt) override;

 private:
  struct notifications_t {
    bool quit;
    bool reload;
    bool update;
    bool force_update;
    queue<string> inputdata;

    notifications_t() : quit(false), reload(false), update(false), force_update(false), inputdata(queue<string>{}) {}
  };

  size_t setup_modules(alignment align);

  bool forward_action(const actions_util::action& cmd);
  bool try_forward_legacy_action(const string& cmd);

  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;
  const config& m_conf;
  eventloop::loop& m_loop;
  unique_ptr<bar> m_bar;
  bool m_has_ipc;
  string m_tray_module_name;

  /**
   * @brief Async handle to notify the eventloop
   *
   * This handle is used to notify the eventloop of changes which are not otherwise covered by other handles.
   * E.g. click actions.
   */
  eventloop::async_handle_t m_notifier{m_loop.handle<eventloop::AsyncHandle>([this]() { notifier_handler(); })};

  /**
   * Notification data for the controller.
   *
   * Triggers, potentially from other threads, update this structure and notify the controller through m_notifier.
   */
  notifications_t m_notifications{};

  /**
   * @brief Protected m_notifications.
   *
   * All accesses to m_notifications must hold this mutex.
   */
  std::mutex m_notification_mutex{};

  /**
   * @brief Destination path of generated snapshot
   */
  string m_snapshot_dst;

  /**
   * @brief Controls weather the output gets printed to stdout
   */
  bool m_writeback{false};

  /**
   * @brief Loaded modules
   */
  vector<module_t> m_modules;

  /**
   * @brief Loaded modules grouped by block
   */
  modulemap_t m_blocks;

  /**
   * @brief Flag to trigger reload after shutdown
   */
  bool m_reload{false};
};

POLYBAR_NS_END
