#include "components/controller.hpp"

#include <csignal>
#include <utility>

#include "components/bar.hpp"
#include "components/builder.hpp"
#include "components/config.hpp"
#include "components/ipc.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "modules/meta/base.hpp"
#include "modules/meta/event_handler.hpp"
#include "modules/meta/factory.hpp"
#include "utils/actions.hpp"
#include "utils/factory.hpp"
#include "utils/inotify.hpp"
#include "utils/process.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "x11/connection.hpp"
#include "x11/extensions/all.hpp"

POLYBAR_NS

array<int, 2> g_eventpipe{{-1, -1}};
sig_atomic_t g_reload{0};
sig_atomic_t g_terminate{0};

void interrupt_handler(int signum) {
  if (g_reload || g_terminate) {
    return;
  }

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
  if (m_conf.has("settings", "throttle-input-for")) {
    m_log.warn(
        "The config parameter 'settings.throttle-input-for' is deprecated, it will be removed in the future. Please "
        "remove it from your config");
  }

  m_swallow_limit = m_conf.deprecated("settings", "eventqueue-swallow", "throttle-output", m_swallow_limit);
  m_swallow_update = m_conf.deprecated("settings", "eventqueue-swallow-time", "throttle-output-for", m_swallow_update);

  if (pipe(g_eventpipe.data()) == 0) {
    m_queuefd[PIPE_READ] = make_unique<file_descriptor>(g_eventpipe[PIPE_READ]);
    m_queuefd[PIPE_WRITE] = make_unique<file_descriptor>(g_eventpipe[PIPE_WRITE]);
  } else {
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
  created_modules += setup_modules(alignment::LEFT);
  created_modules += setup_modules(alignment::CENTER);
  created_modules += setup_modules(alignment::RIGHT);

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
  signal(SIGALRM, SIG_DFL);

  if (g_reload) {
    // Cause SIGUSR1 to be ignored until registered in the new polybar process
    signal(SIGUSR1, SIG_IGN);
  }

  m_log.trace("controller: Detach signal receiver");
  m_sig.detach(this);

  m_log.trace("controller: Stop modules");
  for (auto&& module : m_modules) {
    auto module_name = module->name();
    auto cleanup_ms = time_util::measure([&module] { module->stop(); });
    m_log.info("Deconstruction of %s took %lu ms.", module_name, cleanup_ms);
  }

  m_log.trace("controller: Joining threads");
  for (auto&& t : m_threads) {
    if (t.joinable()) {
      t.join();
    }
  }
}

/**
 * Run the main loop
 */
bool controller::run(bool writeback, string snapshot_dst) {
  m_log.info("Starting application");
  m_log.trace("controller: Main thread id = %i", concurrency_util::thread_id(this_thread::get_id()));

  assert(!m_connection.connection_has_error());

  m_writeback = writeback;
  m_snapshot_dst = move(snapshot_dst);

  m_sig.attach(this);

  size_t started_modules{0};
  for (const auto& module : m_modules) {
    auto evt_handler = dynamic_cast<event_handler_interface*>(&*module);

    if (evt_handler != nullptr) {
      evt_handler->connect(m_connection);
    }

    try {
      m_log.info("Starting %s", module->name());
      module->start();
      started_modules++;
    } catch (const application_error& err) {
      m_log.err("Failed to start '%s' (reason: %s)", module->name(), err.what());
    }
  }

  if (!started_modules) {
    throw application_error("No modules started");
  }

  m_connection.flush();
  m_event_thread = thread(&controller::process_eventqueue, this);

  read_events();

  if (m_event_thread.joinable()) {
    enqueue(make_quit_evt(static_cast<bool>(g_reload)));
    m_event_thread.join();
  }

  m_log.notice("Termination signal received, shutting down...");

  return !g_reload;
}

/**
 * Enqueue event
 */
bool controller::enqueue(event&& evt) {
  if (!m_process_events && evt.type != event_type::QUIT) {
    return false;
  }
  if (!m_queue.enqueue(forward<decltype(evt)>(evt))) {
    m_log.warn("Failed to enqueue event");
    return false;
  }
  // if (write(g_eventpipe[PIPE_WRITE], " ", 1) == -1) {
  //   m_log.err("Failed to write to eventpipe (reason: %s)", strerror(errno));
  // }
  return true;
}

/**
 * Enqueue input data
 */
bool controller::enqueue(string&& input_data) {
  if (!m_inputdata.empty()) {
    m_log.trace("controller: Swallowing input event (pending data)");
  } else {
    m_inputdata = forward<string>(input_data);
    return enqueue(make_input_evt());
  }
  return false;
}

/**
 * Read events from configured file descriptors
 */
void controller::read_events() {
  m_log.info("Entering event loop (thread-id=%lu)", this_thread::get_id());

  int fd_connection{-1};
  int fd_confwatch{-1};
  int fd_ipc{-1};

  vector<int> fds;
  fds.emplace_back(*m_queuefd[PIPE_READ]);
  fds.emplace_back((fd_connection = m_connection.get_file_descriptor()));

  if (m_confwatch) {
    m_log.trace("controller: Attach config watch");
    m_confwatch->attach(IN_MODIFY | IN_IGNORED);
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
    if (events == -1) {
      /*
       * The Interrupt errno is generated when polybar is stopped, so it
       * shouldn't generate an error message
       */
      if (errno != EINTR) {
        m_log.err("select failed in event loop: %s", strerror(errno));
      }

      break;
    }

    if (g_terminate || m_connection.connection_has_error()) {
      break;
    }

    // Process event on the internal fd
    if (m_queuefd[PIPE_READ] && FD_ISSET(static_cast<int>(*m_queuefd[PIPE_READ]), &readfds)) {
      char buffer[BUFSIZ];
      if (read(static_cast<int>(*m_queuefd[PIPE_READ]), &buffer, BUFSIZ) == -1) {
        m_log.err("Failed to read from eventpipe (err: %s)", strerror(errno));
      }
    }

    // Process event on the config inotify watch fd
    unique_ptr<inotify_event> confevent;
    if (fd_confwatch > -1 && FD_ISSET(fd_confwatch, &readfds) && (confevent = m_confwatch->await_match())) {
      if (confevent->mask & IN_IGNORED) {
        // IN_IGNORED: file was deleted or filesystem was unmounted
        //
        // This happens in some configurations of vim when a file is saved,
        // since it is not actually issuing calls to write() but rather
        // moves a file into the original's place after moving the original
        // file to a different location (and subsequently deleting it).
        //
        // We need to re-attach the watch to the new file in this case.
        fds.erase(
            std::remove_if(fds.begin(), fds.end(), [fd_confwatch](int fd) { return fd == fd_confwatch; }), fds.end());
        m_confwatch = inotify_util::make_watch(m_confwatch->path());
        m_confwatch->attach(IN_MODIFY | IN_IGNORED);
        fds.emplace_back((fd_confwatch = m_confwatch->get_file_descriptor()));
      }
      m_log.info("Configuration file changed");
      g_terminate = 1;
      g_reload = 1;
    }

    // Process event on the xcb connection fd
    if (fd_connection > -1 && FD_ISSET(fd_connection, &readfds)) {
      shared_ptr<xcb_generic_event_t> evt{};
      while ((evt = shared_ptr<xcb_generic_event_t>(xcb_poll_for_event(m_connection), free)) != nullptr) {
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
    if (fd_ipc > -1 && FD_ISSET(fd_ipc, &readfds)) {
      m_ipc->receive_message();
      fds.erase(std::remove_if(fds.begin(), fds.end(), [fd_ipc](int fd) { return fd == fd_ipc; }), fds.end());
      fds.emplace_back((fd_ipc = m_ipc->get_file_descriptor()));
    }
  }
}

/**
 * Eventqueue worker loop
 */
void controller::process_eventqueue() {
  m_log.info("Eventqueue worker (thread-id=%lu)", this_thread::get_id());
  if (!m_writeback) {
    m_sig.emit(signals::eventqueue::start{});
  } else {
    // bypass the start eventqueue signal
    m_sig.emit(signals::ui::ready{});
  }

  while (!g_terminate) {
    event evt{};
    m_queue.wait_dequeue(evt);

    if (g_terminate) {
      break;
    } else if (evt.type == event_type::QUIT) {
      if (evt.flag) {
        on(signals::eventqueue::exit_reload{});
      } else {
        on(signals::eventqueue::exit_terminate{});
      }
    } else if (evt.type == event_type::INPUT) {
      process_inputdata();
    } else if (evt.type == event_type::UPDATE && evt.flag) {
      process_update(true);
    } else {
      event next{};
      size_t swallowed{0};
      while (swallowed++ < m_swallow_limit && m_queue.wait_dequeue_timed(next, m_swallow_update)) {
        if (next.type == event_type::QUIT) {
          evt = next;
          break;
        } else if (next.type == event_type::INPUT) {
          evt = next;
          break;
        } else if (evt.type != next.type) {
          enqueue(move(next));
          break;
        } else {
          m_log.trace_x("controller: Swallowing event within timeframe");
          evt = next;
        }
      }

      if (evt.type == event_type::UPDATE) {
        process_update(evt.flag);
      } else if (evt.type == event_type::INPUT) {
        process_inputdata();
      } else if (evt.type == event_type::QUIT) {
        if (evt.flag) {
          on(signals::eventqueue::exit_reload{});
        } else {
          on(signals::eventqueue::exit_terminate{});
        }
      } else if (evt.type == event_type::CHECK) {
        on(signals::eventqueue::check_state{});
      } else {
        m_log.warn("Unknown event type for enqueued event (%d)", evt.type);
      }
    }
  }
}

/**
 * Tries to match the given command to a legacy action string and sends the
 * appropriate new action (and data) to the right module if possible.
 *
 * \returns true iff the given command matches a legacy action string and was
 *          successfully forwarded to a module
 */
bool controller::try_forward_legacy_action(const string& cmd) {
  /*
   * Maps legacy action names to a module type and the new action name in that module.
   *
   * We try to match the old action name as a prefix, and everything after it will also be added to the end of the new
   * action string (for example "mpdseek+5" will be redirected to "seek.+5" in the first mpd module).
   *
   * The action will be delivered to the first module of that type so that it is consistent with existing behavior.
   * If the module does not support the action or no matching module is found, the command is forwarded to the shell.
   *
   * TODO Remove when deprecated action names are removed
   */
// clang-format off
#define A_MAP(old, module_name, event) {old, {string(module_name::TYPE), string(module_name::event)}}

  static const std::unordered_map<string, std::pair<string, const string>> legacy_actions{
    A_MAP("datetoggle", date_module, EVENT_TOGGLE),
#if ENABLE_ALSA
    A_MAP("volup", alsa_module, EVENT_INC),
    A_MAP("voldown", alsa_module, EVENT_DEC),
    A_MAP("volmute", alsa_module, EVENT_TOGGLE),
#endif
#if ENABLE_PULSEAUDIO
    A_MAP("pa_volup", pulseaudio_module, EVENT_INC),
    A_MAP("pa_voldown", pulseaudio_module, EVENT_DEC),
    A_MAP("pa_volmute", pulseaudio_module, EVENT_TOGGLE),
#endif
    A_MAP("xbacklight+", xbacklight_module, EVENT_INC),
    A_MAP("xbacklight-", xbacklight_module, EVENT_DEC),
    A_MAP("backlight+", backlight_module, EVENT_INC),
    A_MAP("backlight-", backlight_module, EVENT_DEC),
#if ENABLE_XKEYBOARD
    A_MAP("xkeyboard/switch", xkeyboard_module, EVENT_SWITCH),
#endif
#if ENABLE_MPD
    A_MAP("mpdplay", mpd_module, EVENT_PLAY),
    A_MAP("mpdpause", mpd_module, EVENT_PAUSE),
    A_MAP("mpdstop", mpd_module, EVENT_STOP),
    A_MAP("mpdprev", mpd_module, EVENT_PREV),
    A_MAP("mpdnext", mpd_module, EVENT_NEXT),
    A_MAP("mpdrepeat", mpd_module, EVENT_REPEAT),
    A_MAP("mpdsingle", mpd_module, EVENT_SINGLE),
    A_MAP("mpdrandom", mpd_module, EVENT_RANDOM),
    A_MAP("mpdconsume", mpd_module, EVENT_CONSUME),
    // Has data
    A_MAP("mpdseek", mpd_module, EVENT_SEEK),
#endif
    // Has data
    A_MAP("xworkspaces-focus=", xworkspaces_module, EVENT_FOCUS),
    A_MAP("xworkspaces-next", xworkspaces_module, EVENT_NEXT),
    A_MAP("xworkspaces-prev", xworkspaces_module, EVENT_PREV),
    // Has data
    A_MAP("bspwm-deskfocus", bspwm_module, EVENT_FOCUS),
    A_MAP("bspwm-desknext", bspwm_module, EVENT_NEXT),
    A_MAP("bspwm-deskprev", bspwm_module, EVENT_PREV),
#if ENABLE_I3
    // Has data
    A_MAP("i3wm-wsfocus-", i3_module, EVENT_FOCUS),
    A_MAP("i3wm-wsnext", i3_module, EVENT_NEXT),
    A_MAP("i3wm-wsprev", i3_module, EVENT_PREV),
#endif
    // Has data
    A_MAP("menu-open-", menu_module, EVENT_OPEN),
    A_MAP("menu-close", menu_module, EVENT_CLOSE),
  };
#undef A_MAP
  // clang-format on

  // Check if any key in the map is a prefix for the `cmd`
  for (const auto& entry : legacy_actions) {
    const auto& key = entry.first;
    if (cmd.compare(0, key.length(), key) == 0) {
      string type = entry.second.first;
      auto data = cmd.substr(key.length());
      string action = entry.second.second;

      // Search for the first module that matches the type for this legacy action
      for (auto&& module : m_modules) {
        if (module->type() == type) {
          auto module_name = module->name_raw();
          if (data.empty()) {
            m_log.warn("The action '%s' is deprecated, use '#%s.%s' instead!", cmd, module_name, action);
          } else {
            m_log.warn("The action '%s' is deprecated, use '#%s.%s.%s' instead!", cmd, module_name, action, data);
          }
          m_log.warn("Consult the 'Actions' page in the polybar documentation for more information.");
          m_log.info(
              "Forwarding legacy action '%s' to module '%s' as '%s' with data '%s'", cmd, module_name, action, data);
          if (!module->input(action, data)) {
            m_log.err("Failed to forward deprecated action to %s module", type);
            // Forward to shell if the module cannot accept the action to not break existing behavior.
            return false;
          }
          // Only deliver to the first matching module.
          return true;
        }
      }
    }
  }

  /*
   * If we couldn't find any matching legacy action, we return false and let
   * the command be forwarded to the shell
   */
  return false;
}

bool controller::forward_action(const actions_util::action& action_triple) {
  string module_name = std::get<0>(action_triple);
  string action = std::get<1>(action_triple);
  string data = std::get<2>(action_triple);

  m_log.info("Forwarding action to modules (module: '%s', action: '%s', data: '%s')", module_name, action, data);

  int num_delivered = 0;

  // Forwards the action to all modules that match the name
  for (auto&& module : m_modules) {
    if (module->name_raw() == module_name) {
      if (!module->input(action, data)) {
        m_log.err("The '%s' module does not support the '%s' action.", module_name, action);
      }

      num_delivered++;
    }
  }

  if (num_delivered == 0) {
    m_log.err("Could not forward action to module: No module named '%s' (action: '%s', data: '%s')", module_name,
        action, data);
  } else {
    m_log.info("Delivered action to %d module%s", num_delivered, num_delivered > 1 ? "s" : "");
  }
  return true;
}

/**
 * Process stored input data
 */
void controller::process_inputdata() {
  if (m_inputdata.empty()) {
    return;
  }

  const string cmd = std::move(m_inputdata);
  m_inputdata = string{};

  m_log.trace("controller: Processing inputdata: %s", cmd);

  // Every command that starts with '#' is considered an action string.
  if (cmd.front() == '#') {
    try {
      this->forward_action(actions_util::parse_action_string(cmd));
    } catch (runtime_error& e) {
      m_log.err("Invalid action string (action: %s, reason: %s)", cmd, e.what());
    }

    return;
  }

  if (this->try_forward_legacy_action(cmd)) {
    return;
  }

  try {
    // Run input as command if it's not an input for a module
    m_log.info("Forwarding command to shell... (input: %s)", cmd);
    m_log.info("Executing shell command: %s", cmd);
    process_util::fork_detached([cmd] { process_util::exec_sh(cmd.c_str()); });
    process_update(true);
  } catch (const application_error& err) {
    m_log.err("controller: Error while forwarding input to shell -> %s", err.what());
  }
}

/**
 * Process eventqueue update event
 */
bool controller::process_update(bool force) {
  const bar_settings& bar{m_bar->settings()};
  string contents;
  string padding_left = builder::get_spacing_format_string(bar.padding.left);
  string padding_right = builder::get_spacing_format_string(bar.padding.right);
  string margin_left = builder::get_spacing_format_string(bar.module_margin.left);
  string margin_right = builder::get_spacing_format_string(bar.module_margin.right);

  builder build{bar};
  build.node(bar.separator);
  string separator{build.flush()};

  for (const auto& block : m_blocks) {
    string block_contents;
    bool is_left = false;
    bool is_center = false;
    bool is_right = false;
    bool is_first = true;

    if (block.first == alignment::LEFT) {
      is_left = true;
    } else if (block.first == alignment::CENTER) {
      is_center = true;
    } else if (block.first == alignment::RIGHT) {
      is_right = true;
    }

    for (const auto& module : block.second) {
      if (!module->running() || !module->visible()) {
        continue;
      }

      string module_contents;

      try {
        module_contents = module->contents();
      } catch (const exception& err) {
        m_log.err("Failed to get contents for \"%s\" (err: %s)", module->name(), err.what());
      }

      if (module_contents.empty()) {
        continue;
      }

      if (!block_contents.empty() && !margin_right.empty()) {
        block_contents += margin_right;
      }

      if (!block_contents.empty() && !separator.empty()) {
        block_contents += separator;
      }

      if (!block_contents.empty() && !margin_left.empty() && !(is_left && is_first)) {
        block_contents += margin_left;
      }

      block_contents.reserve(module_contents.size());
      block_contents += module_contents;

      is_first = false;
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

    contents += block_contents;
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
 * Creates module instances for all the modules in the given alignment block
 */
size_t controller::setup_modules(alignment align) {
  string key;

  switch (align) {
    case alignment::LEFT:
      key = "modules-left";
      break;

    case alignment::CENTER:
      key = "modules-center";
      break;

    case alignment::RIGHT:
      key = "modules-right";
      break;

    case alignment::NONE:
      m_log.err("controller: Tried to setup modules for alignment NONE");
      break;
  }

  string configured_modules;
  if (!key.empty()) {
    configured_modules = m_conf.get(m_conf.section(), key, ""s);
  }

  for (auto& module_name : string_util::split(configured_modules, ' ')) {
    if (module_name.empty()) {
      continue;
    }

    try {
      auto type = m_conf.get("module/" + module_name, "type");

      if (type == ipc_module::TYPE && !m_ipc) {
        throw application_error("Inter-process messaging needs to be enabled");
      }

      auto ptr = make_module(move(type), m_bar->settings(), module_name, m_log);
      module_t module = shared_ptr<modules::module_interface>(ptr);
      ptr = nullptr;

      m_modules.push_back(module);
      m_blocks[align].push_back(module);
    } catch (const std::exception& err) {
      m_log.err("Disabling module \"%s\" (reason: %s)", module_name, err.what());
    }
  }

  return m_modules.size();
}

/**
 * Process broadcast events
 */
bool controller::on(const signals::eventqueue::notify_change&) {
  return enqueue(make_update_evt(false));
}

/**
 * Process forced broadcast events
 */
bool controller::on(const signals::eventqueue::notify_forcechange&) {
  return enqueue(make_update_evt(true));
}

/**
 * Process eventqueue terminate event
 */
bool controller::on(const signals::eventqueue::exit_terminate&) {
  raise(SIGALRM);
  return true;
}

/**
 * Process eventqueue reload event
 */
bool controller::on(const signals::eventqueue::exit_reload&) {
  raise(SIGUSR1);
  return true;
}

/**
 * Process eventqueue check event
 */
bool controller::on(const signals::eventqueue::check_state&) {
  for (const auto& module : m_modules) {
    if (module->running()) {
      return true;
    }
  }
  m_log.warn("No running modules...");
  on(signals::eventqueue::exit_terminate{});
  return true;
}

/**
 * Process ui ready event
 */
bool controller::on(const signals::ui::ready&) {
  m_process_events = true;
  enqueue(make_update_evt(true));

  if (!m_snapshot_dst.empty()) {
    m_threads.emplace_back(thread([&] {
      this_thread::sleep_for(3s);
      m_sig.emit(signals::ui::request_snapshot{move(m_snapshot_dst)});
      enqueue(make_update_evt(true));
    }));
  }

  // let the event bubble
  return false;
}

/**
 * Process ui button press event
 */
bool controller::on(const signals::ui::button_press& evt) {
  string input{evt.cast()};

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
bool controller::on(const signals::ipc::action& evt) {
  string action{evt.cast()};

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
bool controller::on(const signals::ipc::command& evt) {
  string command{evt.cast()};

  if (command.empty()) {
    return false;
  }

  if (command == "quit") {
    enqueue(make_quit_evt(false));
  } else if (command == "restart") {
    enqueue(make_quit_evt(true));
  } else if (command == "hide") {
    m_bar->hide();
  } else if (command == "show") {
    m_bar->show();
  } else if (command == "toggle") {
    m_bar->toggle();
  } else {
    m_log.warn("\"%s\" is not a valid ipc command", command);
  }

  return true;
}

/**
 * Process ipc hook messages
 */
bool controller::on(const signals::ipc::hook& evt) {
  string hook{evt.cast()};

  for (const auto& module : m_modules) {
    if (!module->running()) {
      continue;
    }
    auto ipc = std::dynamic_pointer_cast<ipc_module>(module);
    if (ipc != nullptr) {
      ipc->on_message(hook);
    }
  }

  return true;
}

bool controller::on(const signals::ui::update_background&) {
  enqueue(make_update_evt(true));

  return false;
}

POLYBAR_NS_END
