#include "components/controller.hpp"
#include "common.hpp"
#include "components/bar.hpp"
#include "components/config.hpp"
#include "components/eventloop.hpp"
#include "components/ipc.hpp"
#include "components/logger.hpp"
#include "events/signal.hpp"
#include "modules/meta/factory.hpp"
#include "utils/command.hpp"
#include "utils/factory.hpp"
#include "utils/inotify.hpp"
#include "utils/process.hpp"
#include "utils/string.hpp"
#include "x11/connection.hpp"
#include "x11/xutils.hpp"

POLYBAR_NS

using namespace modules;

/**
 * Create instance
 */
controller::make_type controller::make(string&& path_confwatch, bool enable_ipc, bool writeback) {
  // clang-format off
  return factory_util::unique<controller>(
      connection::make(),
      signal_emitter::make(),
      logger::make(),
      config::make(),
      eventloop::make(),
      bar::make(),
      enable_ipc ? ipc::make() : ipc::make_type{},
      !path_confwatch.empty() ? inotify_util::make_watch(forward<decltype(path_confwatch)>(move(path_confwatch))) : watch_t{},
      writeback);
  // clang-format on
}

/**
 * Construct controller object
 */
controller::controller(connection& conn, signal_emitter& emitter, const logger& logger, const config& config,
    unique_ptr<eventloop>&& eventloop, unique_ptr<bar>&& bar, unique_ptr<ipc>&& ipc, watch_t&& confwatch,
    bool writeback)
    : m_connection(conn)
    , m_sig(emitter)
    , m_log(logger)
    , m_conf(config)
    , m_eventloop(forward<decltype(eventloop)>(eventloop))
    , m_bar(forward<decltype(bar)>(bar))
    , m_ipc(forward<decltype(ipc)>(ipc))
    , m_confwatch(forward<decltype(confwatch)>(confwatch))
    , m_writeback(writeback) {}

/**
 * Deconstruct controller object
 */
controller::~controller() {
  if (m_command) {
    m_log.info("Terminating running shell command");
    m_command.reset();
  }
  if (!m_writeback) {
    m_log.info("Interrupting X event loop");
    m_connection.send_dummy_event(m_connection.root());
  }

  m_log.info("Joining active threads");
  for (auto&& thread_ : m_threads) {
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  m_log.info("Waiting for spawned processes");
  while (process_util::notify_childprocess()) {
    ;
  }

  m_connection.flush();
}

void controller::setup() {
  if (!m_writeback) {
    m_connection.ensure_event_mask(m_connection.root(), XCB_EVENT_MASK_STRUCTURE_NOTIFY);
  }

  string bs{m_conf.section()};
  m_log.trace("controller: Setup user-defined modules");

  for (int i = 0; i < 3; i++) {
    alignment align = static_cast<alignment>(i + 1);
    string confkey;

    if (align == alignment::LEFT) {
      confkey = "modules-left";
    } else if (align == alignment::CENTER) {
      confkey = "modules-center";
    } else if (align == alignment::RIGHT) {
      confkey = "modules-right";
    }

    for (auto& module_name : string_util::split(m_conf.get<string>(bs, confkey, ""), ' ')) {
      if (module_name.empty()) {
        continue;
      }

      try {
        auto type = m_conf.get<string>("module/" + module_name, "type");
        if (type == "custom/ipc" && !m_ipc) {
          throw application_error("Inter-process messaging needs to be enabled");
        }

        unique_ptr<module_interface> module{make_module(move(type), m_bar->settings(), module_name)};

        module->set_update_cb([&] {
          if (m_eventloop && m_running) {
            m_sig.emit(enqueue_update{eventloop_t::make_update_evt(false)});
          }
        });
        module->set_stop_cb([&] {
          if (m_eventloop && m_running) {
            m_sig.emit(enqueue_check{eventloop::make_check_evt()});
          }
        });
        module->setup();

        m_eventloop->add_module(align, move(module));
      } catch (const std::runtime_error& err) {
        m_log.err("Disabling module \"%s\" (reason: %s)", module_name, err.what());
      }
    }
  }

  if (!m_eventloop->module_count()) {
    throw application_error("No modules created");
  }
}

bool controller::run() {
  assert(!m_connection.connection_has_error());

  m_log.info("Starting application");
  m_running = true;

  m_sig.attach(this);

  if (m_confwatch && !m_writeback) {
    m_threads.emplace_back(thread(&controller::wait_for_configwatch, this));
  }
  if (m_ipc) {
    m_threads.emplace_back(thread(&ipc::receive_messages, m_ipc.get()));
  }
  if (!m_writeback) {
    m_threads.emplace_back(thread(&controller::wait_for_xevent, this));
  }
  if (m_eventloop) {
    m_threads.emplace_back(thread(&controller::wait_for_eventloop, this));
  }

  m_log.trace("controller: Wait for signal");
  m_waiting = true;

  sigemptyset(&m_waitmask);
  sigaddset(&m_waitmask, SIGINT);
  sigaddset(&m_waitmask, SIGQUIT);
  sigaddset(&m_waitmask, SIGTERM);
  sigaddset(&m_waitmask, SIGUSR1);
  sigaddset(&m_waitmask, SIGALRM);

  int caught_signal = 0;
  sigwait(&m_waitmask, &caught_signal);

  m_running = false;
  m_waiting = false;

  if (caught_signal == SIGUSR1) {
    m_reload = true;
  }

  m_log.warn("Termination signal received, shutting down...");
  m_log.trace("controller: Caught signal %d", caught_signal);

  m_sig.detach(this);

  if (m_eventloop) {
    // Signal the eventloop, in case it's still running
    m_eventloop->enqueue(eventloop::make_quit_evt(false));

    m_log.trace("controller: Stopping event loop");
    m_eventloop->stop();
  }

  if (m_ipc) {
    m_ipc.reset();
  }

  if (!m_writeback && m_confwatch) {
    m_log.trace("controller: Removing config watch");
    m_confwatch->remove(true);
  }

  return !m_running && !m_reload;
}

const bar_settings controller::opts() const {
  return m_bar->settings();
}

void controller::wait_for_configwatch() {
  try {
    m_log.trace("controller: Attach config watch");
    m_confwatch->attach(IN_MODIFY);

    m_log.trace("controller: Wait for config file inotify event");
    if (m_confwatch->await_match() && m_running) {
      m_log.info("Configuration file changed");
      kill(getpid(), SIGUSR1);
    }
  } catch (const system_error& err) {
    m_log.err(err.what());
    m_log.trace("controller: Reset config watch");
    m_confwatch.reset();
  }
}

void controller::wait_for_xevent() {
  m_log.trace("controller: Listen for X events");
  m_connection.flush();

  while (m_running) {
    try {
      auto evt = m_connection.wait_for_event();
      if (evt && m_running) {
        m_connection.dispatch_event(evt);
      }
    } catch (xpp::connection_error& err) {
      m_log.err("X connection error, terminating... (what: %s)", m_connection.error_str(err.code()));
    } catch (const exception& err) {
      m_log.err("Error in X event loop: %s", err.what());
    }
    if (m_connection.connection_has_error()) {
      break;
    }
  }

  if (m_running) {
    kill(getpid(), SIGTERM);
  }
}

void controller::wait_for_eventloop() {
  m_eventloop->start();

  this_thread::sleep_for(std::chrono::milliseconds{250});

  if (m_running) {
    m_log.trace("controller: eventloop ended, raising SIGALRM");
    kill(getpid(), SIGALRM);
  }
}

bool controller::on(const sig_ev::process_update& evt) {
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

  for (const auto& block : m_eventloop->modules()) {
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

bool controller::on(const sig_ev::process_input& evt) {
  try {
    string input{*evt.data()};

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

bool controller::on(const sig_ev::process_quit&) {
  kill(getpid(), SIGUSR1);
  return false;
}

bool controller::on(const sig_ui::button_press& evt) {
  if (!m_eventloop) {
    return false;
  }

  string input{*evt.data()};

  if (input.empty()) {
    m_log.err("Cannot enqueue empty input");
    return false;
  }

  return m_sig.emit(enqueue_input{move(input)});
}

bool controller::on(const sig_ipc::process_action& evt) {
  ipc_action a{*evt.data()};
  string action{a.payload};
  action.erase(0, strlen(ipc_action::prefix));

  if (action.empty()) {
    m_log.err("Cannot enqueue empty ipc action");
    return false;
  }

  m_log.info("Enqueuing ipc action: %s", action);
  return m_sig.emit(enqueue_input{move(action)});
}

bool controller::on(const sig_ipc::process_command& evt) {
  ipc_command c{*evt.data()};
  string command{c.payload};
  command.erase(0, strlen(ipc_command::prefix));

  if (command.empty()) {
    return false;
  }

  if (command == "quit") {
    m_eventloop->enqueue(eventloop::make_quit_evt(false));
  } else if (command == "restart") {
    m_eventloop->enqueue(eventloop::make_quit_evt(true));
  } else {
    m_log.warn("\"%s\" is not a valid ipc command", command);
  }

  return true;
}

bool controller::on(const sig_ipc::process_hook& evt) {
  const ipc_hook hook{*evt.data()};

  for (const auto& block : m_eventloop->modules()) {
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
