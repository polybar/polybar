#include <csignal>

#include "components/bar.hpp"
#include "components/config.hpp"
#include "components/controller.hpp"
#include "components/ipc.hpp"
#include "components/logger.hpp"
#include "components/renderer.hpp"
#include "components/types.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "modules/meta/factory.hpp"
#include "utils/command.hpp"
#include "utils/factory.hpp"
#include "utils/inotify.hpp"
#include "utils/process.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "x11/connection.hpp"
#include "x11/tray_manager.hpp"
#include "x11/xutils.hpp"

POLYBAR_NS

int g_eventpipe[2]{0, 0};
sig_atomic_t g_reload{0};
sig_atomic_t g_terminate{0};

void interrupt_handler(int signum) {
  g_terminate = 1;
  g_reload = (signum == SIGUSR1);
  if (write(g_eventpipe[PIPE_WRITE], &g_terminate, 1) == -1) {
    throw system_error("Failed to write to eventpipe");
  }
}

/**
 * Build controller instance
 */
controller::make_type controller::make(unique_ptr<ipc>&& ipc, unique_ptr<inotify_watch>&& config_watch) {
  return factory_util::unique<controller>(connection::make(), signal_emitter::make(), logger::make(), config::make(),
      bar::make(), forward<decltype(ipc)>(ipc), forward<decltype(config_watch)>(config_watch));
}

/**
 * Construct controller
 */
controller::controller(connection& conn, signal_emitter& emitter, const logger& logger, const config& config,
    unique_ptr<bar>&& bar, unique_ptr<ipc>&& ipc, unique_ptr<inotify_watch>&& confwatch)
    : m_connection(conn)
    , m_sig(emitter)
    , m_log(logger)
    , m_conf(config)
    , m_bar(forward<decltype(bar)>(bar))
    , m_ipc(forward<decltype(ipc)>(ipc))
    , m_confwatch(forward<decltype(confwatch)>(confwatch)) {
  m_swallow_input = m_conf.get("settings", "throttle-input-for", m_swallow_input);
  m_swallow_limit = m_conf.deprecated("settings", "eventqueue-swallow", "throttle-output", m_swallow_limit);
  m_swallow_update = m_conf.deprecated("settings", "eventqueue-swallow-time", "throttle-output-for", m_swallow_update);

  if (pipe(g_eventpipe) != 0) {
    throw system_error("Failed to create event channel pipes");
  }

  m_log.trace("controller: Install signal handler");
  struct sigaction act {};
  memset(&act, 0, sizeof(act));
  act.sa_handler = &interrupt_handler;
  sigaction(SIGINT, &act, nullptr);
  sigaction(SIGQUIT, &act, nullptr);
  sigaction(SIGTERM, &act, nullptr);
  sigaction(SIGUSR1, &act, nullptr);
  sigaction(SIGALRM, &act, nullptr);

  m_log.trace("controller: Setup user-defined modules");
  size_t created_modules{0};

  for (int i = 0; i < 3; i++) {
    alignment align{static_cast<alignment>(i + 1)};
    string configured_modules;

    if (align == alignment::LEFT) {
      configured_modules = m_conf.get<string>(m_conf.section(), "modules-left", "");
    } else if (align == alignment::CENTER) {
      configured_modules = m_conf.get<string>(m_conf.section(), "modules-center", "");
    } else if (align == alignment::RIGHT) {
      configured_modules = m_conf.get<string>(m_conf.section(), "modules-right", "");
    }

    for (auto& module_name : string_util::split(configured_modules, ' ')) {
      if (module_name.empty()) {
        continue;
      }

      try {
        auto type = m_conf.get<string>("module/" + module_name, "type");

        if (type == "custom/ipc" && !m_ipc) {
          throw application_error("Inter-process messaging needs to be enabled");
        }

        auto module = make_module(move(type), m_bar->settings(), module_name);

        module->set_update_cb([&] { enqueue(make_update_evt(false)); });
        module->set_stop_cb([&] { enqueue(make_check_evt()); });

        m_modules[align].emplace_back(move(module));

        created_modules++;
      } catch (const runtime_error& err) {
        m_log.err("Disabling module \"%s\" (reason: %s)", module_name, err.what());
      }
    }
  }

  if (!created_modules) {
    throw application_error("No modules created");
  }
}

/**
 * Deconstruct controller
 */
controller::~controller() {
  m_log.trace("controller: Uninstall sighandler");
  signal(SIGINT, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGTERM, SIG_DFL);

  m_log.trace("controller: Detach signal receiver");
  m_sig.detach(this);

  m_log.trace("controller: Stop modules");
  for (auto&& block : m_modules) {
    for (auto&& module : block.second) {
      auto module_name = module->name();
      auto cleanup_ms = time_util::measure([&module] {
        module->stop();
        module.reset();
      });
      m_log.info("Deconstruction of %s took %lu ms.", module_name, cleanup_ms);
    }
  }
}

/**
 * Run the main loop
 */
bool controller::run(bool writeback) {
  assert(!m_connection.connection_has_error());

  m_writeback = writeback;

  m_log.info("Starting application");
  m_sig.attach(this);

  size_t started_modules{0};
  for (const auto& block : m_modules) {
    for (const auto& module : block.second) {
      try {
        m_log.info("Starting %s", module->name());
        module->start();
        started_modules++;
      } catch (const application_error& err) {
        m_log.err("Failed to start '%s' (reason: %s)", module->name(), err.what());
      }
    }
  }

  if (!started_modules) {
    throw application_error("No modules started");
  }

  m_connection.flush();

  read_events();

  m_log.warn("Termination signal received, shutting down...");

  return !g_reload;
}

/**
 * Enqueue event
 */
bool controller::enqueue(event&& evt) {
  if (!m_queue.enqueue(move(evt))) {
    m_log.warn("Failed to enqueue event");
    return false;
  }
  if (write(g_eventpipe[PIPE_WRITE], " ", 1) == -1) {
    m_log.err("Failed to write to eventpipe (reason: %s)", strerror(errno));
  }
  return true;
}

/**
 * Enqueue input data
 */
bool controller::enqueue(string&& input_data) {
  if (!m_inputdata.empty()) {
    m_log.trace("controller: Swallowing input event (pending data)");
  } else if (chrono::system_clock::now() - m_swallow_input < m_lastinput) {
    m_log.trace("controller: Swallowing input event (throttled)");
  } else {
    m_inputdata = move(input_data);
    return enqueue(make_input_evt());
  }
  return false;
}

/**
 * Read events from configured file descriptors
 */
void controller::read_events() {
  int fd_confwatch{0};
  int fd_connection{0};
  int fd_event{0};
  int fd_ipc{0};

  vector<int> fds;
  fds.emplace_back((fd_event = g_eventpipe[PIPE_READ]));
  fds.emplace_back((fd_connection = m_connection.get_file_descriptor()));

  if (m_confwatch) {
    m_log.trace("controller: Attach config watch");
    m_confwatch->attach(IN_MODIFY);
    fds.emplace_back((fd_confwatch = m_confwatch->get_file_descriptor()));
  }

  if (m_ipc) {
    fds.emplace_back((fd_ipc = m_ipc->get_file_descriptor()));
  }

  while (!g_terminate) {
    fd_set readfds{};
    FD_ZERO(&readfds);

    int maxfd{0};
    for (auto&& fd : fds) {
      FD_SET(fd, &readfds);
      maxfd = std::max(maxfd, fd);
    }

    // Wait until event is ready on one of the configured streams
    int events = select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);

    // Check for errors
    if (events == -1 || g_terminate || m_connection.connection_has_error()) {
      break;
    }

    // Process event on the internal fd
    if (fd_event && FD_ISSET(fd_event, &readfds)) {
      process_eventqueue();
      char buffer[BUFSIZ]{'\0'};
      if (read(fd_event, &buffer, BUFSIZ) == -1) {
        m_log.err("Failed to read from eventpipe (err: %s)", strerror(errno));
      }
    }

    // Process event on the config inotify watch fd
    if (fd_confwatch && FD_ISSET(fd_confwatch, &readfds) && m_confwatch->await_match()) {
      m_log.info("Configuration file changed");
      g_terminate = 1;
      g_reload = 1;
    }

    // Process event on the xcb connection fd
    if (fd_connection && FD_ISSET(fd_connection, &readfds)) {
      shared_ptr<xcb_generic_event_t> evt;
      while ((evt = m_connection.poll_for_event())) {
        try {
          m_connection.dispatch_event(evt);
        } catch (xpp::connection_error& err) {
          m_log.err("X connection error, terminating... (what: %s)", m_connection.error_str(err.code()));
        } catch (const exception& err) {
          m_log.err("Error in X event loop: %s", err.what());
        }
      }
    }

    // Process event on the ipc fd
    if (fd_ipc && FD_ISSET(fd_ipc, &readfds)) {
      m_ipc->receive_message();
      fds.erase(std::remove_if(fds.begin(), fds.end(), [fd_ipc](int fd) { return fd == fd_ipc; }));
      fds.emplace_back((fd_ipc = m_ipc->get_file_descriptor()));
    }
  }
}

/**
 * Dequeue items from the eventqueue
 */
void controller::process_eventqueue() {
  event evt{};

  if (!m_queue.try_dequeue(evt)) {
    return m_log.err("Failed to dequeue event");
  }

  if (evt.type == static_cast<uint8_t>(event_type::INPUT)) {
    process_inputdata();
  } else {
    event next{};
    size_t swallowed{0};
    while (swallowed++ < m_swallow_limit && m_queue.wait_dequeue_timed(next, m_swallow_update)) {
      if (next.type == static_cast<uint8_t>(event_type::QUIT)) {
        evt = next;
        break;
      } else if (next.type == static_cast<uint8_t>(event_type::INPUT)) {
        evt = next;
        break;
      } else if (evt.type != next.type) {
        m_queue.try_enqueue(move(next));
        break;
      } else {
        m_log.trace_x("controller: Swallowing event within timeframe");
        evt = next;
      }
    }

    if (evt.type == static_cast<uint8_t>(event_type::UPDATE)) {
      m_sig.emit(sig_ev::process_update{make_update_evt(evt.flag)});
    } else if (evt.type == static_cast<uint8_t>(event_type::INPUT)) {
      process_inputdata();
    } else if (evt.type == static_cast<uint8_t>(event_type::QUIT)) {
      m_sig.emit(sig_ev::process_quit{make_quit_evt(evt.flag)});
    } else if (evt.type == static_cast<uint8_t>(event_type::CHECK)) {
      m_sig.emit(sig_ev::process_check{});
    } else {
      m_log.warn("Unknown event type for enqueued event (%d)", evt.type);
    }
  }
}

/**
 * Process stored input data
 */
void controller::process_inputdata() {
  if (!m_inputdata.empty()) {
    m_sig.emit(sig_ev::process_input{move(m_inputdata)});
    m_lastinput = chrono::time_point_cast<decltype(m_swallow_input)>(chrono::system_clock::now());
    m_inputdata.clear();
  }
}

/**
 * Process eventqueue update event
 */
bool controller::on(const sig_ev::process_update& evt) {
  bool force{evt.data()->flag};

  if (!m_bar) {
    return false;
  }

  const bar_settings& bar{m_bar->settings()};
  string contents;
  string separator{bar.separator};
  string padding_left(bar.padding.left, ' ');
  string padding_right(bar.padding.right, ' ');
  string margin_left(bar.module_margin.left, ' ');
  string margin_right(bar.module_margin.right, ' ');

  for (const auto& block : m_modules) {
    string block_contents;
    bool is_left = false;
    bool is_center = false;
    bool is_right = false;

    if (block.first == alignment::LEFT) {
      is_left = true;
    } else if (block.first == alignment::CENTER) {
      is_center = true;
    } else if (block.first == alignment::RIGHT) {
      is_right = true;
    }

    for (const auto& module : block.second) {
      string module_contents{module->contents()};

      if (module_contents.empty()) {
        continue;
      }

      if (!block_contents.empty() && !margin_right.empty()) {
        block_contents += margin_right;
      }

      if (!block_contents.empty() && !separator.empty()) {
        block_contents += separator;
      }

      if (!block_contents.empty() && !margin_left.empty() && !(is_left && module == block.second.front())) {
        block_contents += margin_left;
      }

      block_contents += module_contents;
    }

    if (block_contents.empty()) {
      continue;
    } else if (is_left) {
      contents += "%{l}";
      contents += padding_left;
    } else if (is_center) {
      contents += "%{c}";
    } else if (is_right) {
      contents += "%{r}";
      block_contents += padding_right;
    }

    // Strip unnecessary reset tags
    block_contents = string_util::replace_all(block_contents, "T-}%{T", "T");
    block_contents = string_util::replace_all(block_contents, "B-}%{B#", "B#");
    block_contents = string_util::replace_all(block_contents, "F-}%{F#", "F#");
    block_contents = string_util::replace_all(block_contents, "U-}%{U#", "U#");
    block_contents = string_util::replace_all(block_contents, "u-}%{u#", "u#");
    block_contents = string_util::replace_all(block_contents, "o-}%{o#", "o#");

    // Join consecutive tags
    contents += string_util::replace_all(block_contents, "}%{", " ");
  }

  try {
    if (!m_writeback) {
      m_bar->parse(move(contents), force);
    } else {
      std::cout << contents << std::endl;
    }
  } catch (const exception& err) {
    m_log.err("Failed to update bar contents (reason: %s)", err.what());
  }

  return true;
}

/**
 * Process eventqueue input event
 */
bool controller::on(const sig_ev::process_input& evt) {
  try {
    string input{*evt()};

    for (auto&& block : m_modules) {
      for (auto&& module : block.second) {
        if (module->receive_events() && module->handle_event(input)) {
          return true;
        }
      }
    }

    m_log.warn("Input event \"%s\" was rejected by all modules, passing to shell...", input);

    if (m_command) {
      m_log.warn("Terminating previous shell command");
      m_command->terminate();
    }

    m_log.info("Executing shell command: %s", input);

    m_command = command_util::make_command(move(input));
    m_command->exec();
    m_command.reset();
  } catch (const application_error& err) {
    m_log.err("controller: Error while forwarding input to shell -> %s", err.what());
  }

  return true;
}

/**
 * Process eventqueue quit event
 */
bool controller::on(const sig_ev::process_quit& evt) {
  bool reload{evt.data()->flag};
  raise(reload ? SIGUSR1 : SIGALRM);
  return true;
}

/**
 * Process eventqueue check event
 */
bool controller::on(const sig_ev::process_check&) {
  for (const auto& block : m_modules) {
    for (const auto& module : block.second) {
      if (module->running()) {
        return true;
      }
    }
  }
  m_log.warn("No running modules...");
  enqueue(make_quit_evt(false));
  return true;
}

/**
 * Process ui button press event
 */
bool controller::on(const sig_ui::button_press& evt) {
  string input{*evt.data()};

  if (input.empty()) {
    m_log.err("Cannot enqueue empty input");
    return false;
  }

  enqueue(move(input));
  return true;
}

/**
 * Process ipc action messages
 */
bool controller::on(const sig_ipc::process_action& evt) {
  ipc_action a{*evt.data()};
  string action{a.payload};
  action.erase(0, strlen(ipc_action::prefix));

  if (action.empty()) {
    m_log.err("Cannot enqueue empty ipc action");
    return false;
  }

  m_log.info("Enqueuing ipc action: %s", action);
  enqueue(move(action));
  return true;
}

/**
 * Process ipc command messages
 */
bool controller::on(const sig_ipc::process_command& evt) {
  ipc_command c{*evt.data()};
  string command{c.payload};
  command.erase(0, strlen(ipc_command::prefix));

  if (command.empty()) {
    return false;
  }

  if (command == "quit") {
    enqueue(make_quit_evt(false));
  } else if (command == "restart") {
    enqueue(make_quit_evt(true));
  } else {
    m_log.warn("\"%s\" is not a valid ipc command", command);
  }

  return true;
}

/**
 * Process ipc hook messages
 */
bool controller::on(const sig_ipc::process_hook& evt) {
  const ipc_hook hook{*evt.data()};

  for (const auto& block : m_modules) {
    for (const auto& module : block.second) {
      auto ipc = dynamic_cast<ipc_module*>(module.get());
      if (ipc != nullptr) {
        ipc->on_message(hook);
      }
    }
  }

  return true;
}

POLYBAR_NS_END
