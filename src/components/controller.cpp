#include <chrono>
#include <mutex>

#include "components/bar.hpp"
#include "components/config.hpp"
#include "components/controller.hpp"
#include "components/eventloop.hpp"
#include "components/ipc.hpp"
#include "components/logger.hpp"
#include "components/signals.hpp"
#include "modules/backlight.hpp"
#include "modules/battery.hpp"
#include "modules/bspwm.hpp"
#include "modules/counter.hpp"
#include "modules/cpu.hpp"
#include "modules/date.hpp"
#include "modules/fs.hpp"
#include "modules/ipc.hpp"
#include "modules/memory.hpp"
#include "modules/menu.hpp"
#include "modules/script.hpp"
#include "modules/temperature.hpp"
#include "modules/text.hpp"
#include "modules/xbacklight.hpp"
#include "modules/xwindow.hpp"
#include "modules/xworkspaces.hpp"
#include "utils/process.hpp"
#include "utils/string.hpp"

#if ENABLE_I3
#include "modules/i3.hpp"
#endif
#if ENABLE_MPD
#include "modules/mpd.hpp"
#endif
#if ENABLE_NETWORK
#include "modules/network.hpp"
#endif
#if ENABLE_ALSA
#include "modules/volume.hpp"
#endif
#if WITH_XKB
#include "modules/xkeyboard.hpp"
#endif

#if not(ENABLE_I3 && ENABLE_MPD && ENABLE_NETWORK && ENABLE_ALSA && WITH_XKB)
#include "modules/unsupported.hpp"
#endif

POLYBAR_NS

using namespace modules;

namespace chrono = std::chrono;

/**
 * Configure injection module
 */
di::injector<unique_ptr<controller>> configure_controller(watch_t& confwatch) {
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

/**
 * Stop modules and cleanup X components,
 * threads and spawned processes
 */
controller::~controller() {
  pthread_sigmask(SIG_UNBLOCK, &m_blockmask, nullptr);

  sigset_t sig;
  sigemptyset(&sig);
  sigaddset(&sig, SIGALRM);
  pthread_sigmask(SIG_BLOCK, &sig, nullptr);

  if (m_command) {
    m_log.info("Terminating running shell command");
    m_command.reset();
  }

  if (m_bar) {
    m_log.info("Deconstructing bar");
    m_bar.reset();
  }

  if (m_ipc) {
    m_log.info("Deconstructing ipc");
    m_ipc.reset();
  }

  if (m_eventloop) {
    m_log.info("Deconstructing eventloop");
    m_eventloop.reset();
  }

  if (!m_writeback) {
    m_log.info("Interrupting X event loop");
    m_connection.send_dummy_event(m_connection.root());
  }

  m_log.info("Joining active threads");
  for (auto&& th : m_threads) {
    if (th.second.joinable()) {
      th.second.join();
    }
  }

  m_log.info("Waiting for spawned processes");
  while (process_util::notify_childprocess()) {
    ;
  }

  m_connection.flush();
}

/**
 * Initialize components and setup X environment
 */
void controller::bootstrap(bool writeback, bool dump_wmname) {
  // Add all signals to the block mask
  sigfillset(&m_blockmask);

  if (pthread_sigmask(SIG_BLOCK, &m_blockmask, nullptr) == -1) {
    throw system_error("Failed to block signals");
  }

  m_writeback = writeback;

  m_log.trace("controller: Initialize X atom cache");
  m_connection.preload_atoms();

  m_log.trace("controller: Query X extension data");
  m_connection.query_extensions();

  if (m_conf.get<bool>(m_conf.bar_section(), "enable-ipc", false)) {
    m_log.trace("controller: Create IPC handler");
    m_ipc = configure_ipc().create<decltype(m_ipc)>();
    m_ipc->attach_callback(bind(&controller::on_ipc_action, this, placeholders::_1));
  } else {
    m_log.info("Inter-process messaging disabled");
  }

  // Listen for events on the root window to be able to
  // break the blocking wait call when cleaning up
  m_log.trace("controller: Listen for events on the root window");
  const uint32_t value_list[2]{XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY};
  m_connection.change_window_attributes_checked(m_connection.root(), XCB_CW_EVENT_MASK, value_list);

  m_log.trace("controller: Setup bar");
  m_bar->bootstrap(m_writeback || dump_wmname);
  m_bar->bootstrap_tray();

  if (dump_wmname) {
    std::cout << m_bar->settings().wmname << std::endl;
    return;
  }

  m_log.trace("controller: Attach eventloop update callback");
  m_eventloop->set_update_cb(bind(&controller::on_update, this, placeholders::_1));

  if (!m_writeback) {
    m_log.trace("controller: Attach eventloop input callback");
    g_signals::bar::action_click = bind(&controller::on_mouse_event, this, placeholders::_1);
    m_eventloop->set_input_db(bind(&controller::on_unrecognized_action, this, placeholders::_1));
  }

  m_log.trace("controller: Setup user-defined modules");
  bootstrap_modules();
}

/**
 * Launch the controller
 */
bool controller::run() {
  assert(!m_connection.connection_has_error());

  m_log.info("Starting application");
  m_running = true;

  if (m_confwatch && !m_writeback) {
    m_threads[thread_role::CONF_LISTENER] = thread(&controller::wait_for_configwatch, this);
  }

  // Start ipc receiver if its enabled
  if (m_conf.get<bool>(m_conf.bar_section(), "enable-ipc", false)) {
    m_threads[thread_role::IPC_LISTENER] = thread(&ipc::receive_messages, m_ipc.get());
  }

  // Listen for X events in separate thread
  if (!m_writeback) {
    m_threads[thread_role::EVENT_QUEUE_X] = thread(&controller::wait_for_xevent, this);
  }

  // Start event loop
  if (m_eventloop) {
    m_threads[thread_role::EVENT_QUEUE] = thread(&controller::wait_for_eventloop, this);
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

  if (m_eventloop) {
    m_log.trace("controller: Stopping event loop");
    m_eventloop->stop();
  }

  if (!m_writeback && m_confwatch) {
    m_log.trace("controller: Removing config watch");
    m_confwatch->remove(true);
  }

  return !m_running && !m_reload;
}

/**
 * Listen for changes to the config file
 */
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

/**
 * Wait for X events and forward them to
 * the event registry
 */
void controller::wait_for_xevent() {
  m_log.trace("controller: Listen for X events");

  m_connection.flush();

  shared_ptr<xcb_generic_event_t> evt;
  while (m_running) {
    try {
      if ((evt = m_connection.wait_for_event()) != nullptr && m_running) {
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

/**
 * Start event loop and wait for it to finish
 */
void controller::wait_for_eventloop() {
  m_eventloop->start();
  m_eventloop->wait();

  this_thread::sleep_for(250ms);

  if (m_running) {
    m_log.trace("controller: eventloop ended, raising SIGALRM");
    kill(getpid(), SIGALRM);
  }
}

/**
 * Create and initialize bar modules
 */
void controller::bootstrap_modules() {
  const bar_settings bar{m_bar->settings()};
  string bs{m_conf.bar_section()};

  for (int i = 0; i < 3; i++) {
    alignment align = static_cast<alignment>(i + 1);
    string confkey;

    switch (align) {
      case alignment::LEFT:
        confkey = "modules-left";
        break;
      case alignment::CENTER:
        confkey = "modules-center";
        break;
      case alignment::RIGHT:
        confkey = "modules-right";
        break;
      default:
        break;
    }

    for (auto& module_name : string_util::split(m_conf.get<string>(bs, confkey, ""), ' ')) {
      if (module_name.empty()) {
        continue;
      }

      try {
        auto type = m_conf.get<string>("module/" + module_name, "type");
        module_t module;

        if (type == "internal/counter") {
          module.reset(new counter_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/backlight") {
          module.reset(new backlight_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/battery") {
          module.reset(new battery_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/bspwm") {
          module.reset(new bspwm_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/cpu") {
          module.reset(new cpu_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/date") {
          module.reset(new date_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/fs") {
          module.reset(new fs_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/memory") {
          module.reset(new memory_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/i3") {
          module.reset(new i3_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/mpd") {
          module.reset(new mpd_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/volume") {
          module.reset(new volume_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/network") {
          module.reset(new network_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/temperature") {
          module.reset(new temperature_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/xbacklight") {
          module.reset(new xbacklight_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/xkeyboard") {
          module.reset(new xkeyboard_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/xwindow") {
          module.reset(new xwindow_module(bar, m_log, m_conf, module_name));
        } else if (type == "internal/xworkspaces") {
          module.reset(new xworkspaces_module(bar, m_log, m_conf, module_name));
        } else if (type == "custom/text") {
          module.reset(new text_module(bar, m_log, m_conf, module_name));
        } else if (type == "custom/script") {
          module.reset(new script_module(bar, m_log, m_conf, module_name));
        } else if (type == "custom/menu") {
          module.reset(new menu_module(bar, m_log, m_conf, module_name));
        } else if (type == "custom/ipc") {
          if (!m_ipc) {
            throw application_error("Inter-process messaging needs to be enabled");
          }
          module.reset(new ipc_module(bar, m_log, m_conf, module_name));
          m_ipc->attach_callback(
              bind(&ipc_module::on_message, static_cast<ipc_module*>(module.get()), placeholders::_1));
        } else {
          throw application_error("Unknown module: " + module_name);
        }

        module->set_update_cb(
            bind(&eventloop::enqueue, m_eventloop.get(), eventloop::entry_t{static_cast<uint8_t>(event_type::UPDATE)}));
        module->set_stop_cb(
            bind(&eventloop::enqueue, m_eventloop.get(), eventloop::entry_t{static_cast<uint8_t>(event_type::CHECK)}));
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

/**
 * Callback for received ipc actions
 */
void controller::on_ipc_action(const ipc_action& message) {
  if (!m_eventloop) {
    return;
  }

  string action = message.payload.substr(strlen(ipc_action::prefix));

  if (action.size() >= sizeof(input_event::data)) {
    m_log.warn("Ignoring input event (size)");
  } else if (action.empty()) {
    m_log.err("Cannot enqueue empty IPC action");
  } else {
    m_log.info("Enqueuing IPC action: %s", action);
    m_eventloop->enqueue(eventloop::make(input_event{}, action));
  }
}

/**
 * Callback for clicked bar actions
 */
void controller::on_mouse_event(const string& input) {
  if (!m_eventloop) {
    return;
  } else if (input.length() >= sizeof(input_event::data)) {
    m_log.warn("Ignoring input event (size)");
  } else if (!m_eventloop->enqueue_delayed(eventloop::make(input_event{}, input))) {
    m_log.trace_x("controller: Dispatcher busy");
  }
}

/**
 * Callback for actions not handled internally by a module
 */
void controller::on_unrecognized_action(string input) {
  try {
    if (m_command) {
      m_log.warn("Terminating previous shell command");
      m_command->terminate();
    }

    m_log.info("Executing shell command: %s", input);

    m_command = command_util::make_command(input);
    m_command->exec();
    m_command.reset();
  } catch (const application_error& err) {
    m_log.err("controller: Error while forwarding input to shell -> %s", err.what());
  }
}

/**
 * Callback for module content update
 */
void controller::on_update(bool force) {
  if (!m_bar) {
    return;
  }

  const bar_settings& bar{m_bar->settings()};

  string contents;
  string separator{bar.separator};

  string padding_left(bar.padding.left, ' ');
  string padding_right(bar.padding.right, ' ');

  auto margin_left = bar.module_margin.left;
  auto margin_right = bar.module_margin.right;

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
      auto module_contents = module->contents();

      if (module_contents.empty()) {
        continue;
      }

      if (!block_contents.empty() && !(is_right && module == block.second.back())) {
        block_contents += string(margin_right, ' ');
      }

      if (!block_contents.empty() && !separator.empty()) {
        block_contents += separator;
      }

      if (!(is_left && module == block.second.front())) {
        block_contents += string(margin_left, ' ');
      }

      block_contents += module->contents();
    }

    if (block_contents.empty()) {
      continue;
    }

    if (is_left) {
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

  if (m_writeback) {
    std::cout << contents << std::endl;
    return;
  }

  try {
    if (!m_trayactivated) {
      m_trayactivated = true;
      m_bar->activate_tray();
    }
  } catch (const exception& err) {
    m_log.err("Failed to active tray manager (reason: %s)", err.what());
  }

  try {
    m_bar->parse(contents, force);
  } catch (const exception& err) {
    m_log.err("Failed to update bar contents (reason: %s)", err.what());
  }
}

POLYBAR_NS_END
