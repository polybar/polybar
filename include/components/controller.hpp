#pragma once

#include <thread>

#include "common.hpp"
#include "components/bar.hpp"
#include "components/config.hpp"
#include "components/logger.hpp"
#include "components/signals.hpp"
#include "components/x11/connection.hpp"
#include "components/x11/randr.hpp"
#include "components/x11/tray.hpp"
#include "components/x11/types.hpp"
#include "config.hpp"
#include "utils/command.hpp"
#include "utils/inotify.hpp"
#include "utils/process.hpp"
#include "utils/socket.hpp"
#include "utils/throttle.hpp"

#include "modules/backlight.hpp"
#include "modules/xbacklight.hpp"
#include "modules/battery.hpp"
#include "modules/bspwm.hpp"
#include "modules/counter.hpp"
#include "modules/cpu.hpp"
#include "modules/date.hpp"
#include "modules/memory.hpp"
#include "modules/menu.hpp"
#include "modules/script.hpp"
#include "modules/text.hpp"
#include "modules/unsupported.hpp"
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

LEMONBUDDY_NS

using namespace modules;
using module_t = unique_ptr<module_interface>;

class controller {
 public:
  /**
   * Construct controller
   */
  explicit controller(connection& conn, const logger& logger, const config& config,
      unique_ptr<bar> bar, unique_ptr<traymanager> tray, inotify_watch_t& confwatch)
      : m_connection(conn)
      , m_log(logger)
      , m_conf(config)
      , m_bar(forward<decltype(bar)>(bar))
      , m_traymanager(forward<decltype(tray)>(tray))
      , m_confwatch(confwatch) {}

  /**
   * Stop modules and cleanup X components,
   * threads and spawned processes
   */
  ~controller() noexcept {
    if (!m_mutex.try_lock_for(3s)) {
      m_log.warn("Failed to acquire lock for 3s... Forcing shutdown using SIGKILL");
      raise(SIGKILL);
    }

    std::lock_guard<std::timed_mutex> guard(m_mutex, std::adopt_lock);

    m_log.trace("controller: Stop modules");
    for (auto&& block : m_modules) {
      for (auto&& module : block.second) {
        module->stop();
      }
    }

    if (m_traymanager) {
      m_log.trace("controller: Deactivate tray manager");
      m_traymanager->deactivate();
    }

    m_log.trace("controller: Deconstruct bar instance");
    g_signals::bar::action_click = nullptr;
    m_bar.reset();

    m_log.trace("controller: Interrupt X event loop");
    m_connection.send_dummy_event(m_connection.root());

    if (m_confwatch) {
      try {
        m_log.trace("controller: Remove config watch");
        m_confwatch->remove();
      } catch (const system_error& err) {
      }
    }

    if (!m_threads.empty()) {
      m_log.trace("controller: Join active threads");
      for (auto&& thread : m_threads) {
        if (thread.joinable())
          thread.join();
      }
    }

    m_log.trace("controller: Wait for spawned processes");
    while (process_util::notify_childprocess())
      ;

    m_connection.flush();
  }

  /**
   * Setup X environment
   */
  auto bootstrap(bool to_stdout = false, bool dump_wmname = false) {
    m_stdout = to_stdout;

    m_log.trace("controller: Initialize X atom cache");
    m_connection.preload_atoms();

    m_log.trace("controller: Query X extension data");
    m_connection.query_extensions();

    // const auto& damage_ext = m_connection.extension<xpp::damage::extension>();
    // m_log.trace("controller: Found 'Damage' (first_event: %i, first_error: %i)",
    //     damage_ext->first_event, damage_ext->first_error);

    // const auto& render_ext = m_connection.extension<xpp::render::extension>();
    // m_log.trace("controller: Found 'Render' (first_event: %i, first_error: %i)",
    //     render_ext->first_event, render_ext->first_error);

    const auto& randr_ext = m_connection.extension<xpp::randr::extension>();
    m_log.trace("controller: Found 'RandR' (first_event: %i, first_error: %i)",
        randr_ext->first_event, randr_ext->first_error);

    // Listen for events on the root window to be able to
    // break the blocking wait call when cleaning up
    m_log.trace("controller: Listen for events on the root window");

    try {
      const uint32_t value_list[1]{XCB_EVENT_MASK_STRUCTURE_NOTIFY};
      m_connection.change_window_attributes_checked(
          m_connection.root(), XCB_CW_EVENT_MASK, value_list);
    } catch (const std::exception& err) {
      throw application_error("Failed to change root window event mask: " + string{err.what()});
    }

    try {
      m_log.trace("controller: Setup bar renderer");
      m_bar->bootstrap(m_stdout || dump_wmname);
      if (dump_wmname) {
        std::cout << m_bar->settings().wmname << std::endl;
        return;
      } else if (!to_stdout) {
        g_signals::bar::action_click = bind(&controller::on_module_click, this, std::placeholders::_1);
      }
    } catch (const std::exception& err) {
      throw application_error("Failed to setup bar renderer: " + string{err.what()});
    }

    try {
      if (m_stdout) {
        m_log.trace("controller: Disabling tray (reason: stdout mode)");
        m_traymanager.reset();
      } else if (m_bar->tray().align == alignment::NONE) {
        m_log.trace("controller: Disabling tray (reason: tray-position)");
        m_traymanager.reset();
      } else {
        m_log.trace("controller: Setup tray manager");
        m_traymanager->bootstrap(m_bar->tray());
      }
    } catch (const std::exception& err) {
      m_log.err(err.what());
      m_log.warn("controller: Disabling tray...");
      m_traymanager.reset();
    }

    m_log.trace("main: Setup bar modules");
    bootstrap_modules();

    // Allow <throttle_limit>  ticks within <throttle_ms> timeframe
    const auto throttle_limit = m_conf.get<unsigned int>("settings", "throttle-limit", 3);
    const auto throttle_ms = chrono::duration<double, std::milli>(
        m_conf.get<unsigned int>("settings", "throttle-ms", 60));
    m_throttler = throttle_util::make_throttler(throttle_limit, throttle_ms);
  }

  /**
   * Launch the controller
   */
  auto run() {
    assert(!m_connection.connection_has_error());

    m_log.info("Starting application...");
    m_running = true;

    install_sigmask();
    install_confwatch();

    m_threads.emplace_back([this] {
      m_connection.flush();

      m_log.trace("controller: Start modules");
      for (auto&& block : m_modules) {
        for (auto&& module : block.second) {
          try {
            module->start();
          } catch (const application_error& err) {
            m_log.err("Failed to start '%s' (reason: %s)", module->name(), err.what());
          }

          // Offset the initial broadcasts by 25ms to
          // avoid the updates from being ignored by the throttler
          this_thread::sleep_for(25ms);
        }
      }

      if (m_stdout) {
        m_log.trace("controller: Ignoring tray manager (reason: stdout mode)");
        m_log.trace("controller: Ignoring X event loop (reason: stdout mode)");
        return;
      }

      if (m_traymanager) {
        try {
          m_log.trace("controller: Activate tray manager");
          m_traymanager->activate();
        } catch (const std::exception& err) {
          m_log.err(err.what());
          m_log.err("controller: Failed to activate tray manager...");
        }
      }

      m_connection.flush();

      m_log.trace("controller: Listen for X events");
      while (m_running) {
        auto evt = m_connection.wait_for_event();

        if (evt != nullptr)
          m_connection.dispatch_event(evt);
      }
    });

    wait();

    m_running = false;

    return !m_reload;
  }

  /**
   * Block execution until a defined signal is raised
   */
  void wait() {
    m_log.trace("controller: Wait for signal");

    int caught_signal = 0;
    sigwait(&m_waitmask, &caught_signal);

    m_log.warn("Termination signal received, shutting down...");
    m_log.trace("controller: Caught signal %d", caught_signal);

    m_reload = (caught_signal == SIGUSR1);
  }

  /**
   * Configure injection module
   */
  static di::injector<unique_ptr<controller>> configure(inotify_watch_t& confwatch) {
    // clang-format off
    return di::make_injector(di::bind<controller>().to<controller>(),
        di::bind<>().to(confwatch),
        connection::configure(),
        logger::configure(), config::configure(),
        bar::configure(),
        traymanager::configure());
    // clang-format on
  }

 protected:
  /**
   * Set signal mask for the current and future threads
   */
  void install_sigmask() {
    if (!m_running)
      return;

    m_log.trace("controller: Set sigmask for current and future threads");

    sigemptyset(&m_waitmask);
    sigaddset(&m_waitmask, SIGINT);
    sigaddset(&m_waitmask, SIGQUIT);
    sigaddset(&m_waitmask, SIGTERM);
    sigaddset(&m_waitmask, SIGUSR1);

    if (pthread_sigmask(SIG_BLOCK, &m_waitmask, nullptr) == -1)
      throw system_error();
  }

  /**
   * Listen for changes to the config file
   */
  void install_confwatch() {
    if (!m_running)
      return;

    if (!m_confwatch) {
      m_log.trace("controller: Config watch not set, skip...");
      return;
    }

    m_threads.emplace_back([this] {
      this_thread::sleep_for(1s);

      try {
        if (!m_running)
          return;

        m_log.trace("controller: Attach config watch");
        m_confwatch->attach(IN_MODIFY);

        m_log.trace("controller: Wait for config file inotify event");
        m_confwatch->get_event();

        if (!m_running)
          return;

        m_log.info("Configuration file changed...");
        kill(getpid(), SIGUSR1);
      } catch (const system_error& err) {
        m_log.err(err.what());
        m_log.trace("controller: Reset config watch");
        m_confwatch.reset();
      }
    });
  }

  /**
   * Create and initialize bar modules
   */
  void bootstrap_modules() {
    m_modules.emplace(alignment::LEFT, vector<module_t>{});
    m_modules.emplace(alignment::CENTER, vector<module_t>{});
    m_modules.emplace(alignment::RIGHT, vector<module_t>{});

    size_t module_count = 0;

    for (auto& block : m_modules) {
      string bs{m_conf.bar_section()};
      string confkey;

      switch (block.first) {
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
        auto type = m_conf.get<string>("module/" + module_name, "type");
        auto bar = m_bar->settings();
        auto& modules = block.second;

        if (type == "internal/counter")
          modules.emplace_back(new counter_module(bar, m_log, m_conf, module_name));
        else if (type == "internal/backlight")
          modules.emplace_back(new backlight_module(bar, m_log, m_conf, module_name));
        else if (type == "internal/xbacklight")
          modules.emplace_back(new xbacklight_module(bar, m_log, m_conf, module_name));
        else if (type == "internal/battery")
          modules.emplace_back(new battery_module(bar, m_log, m_conf, module_name));
        else if (type == "internal/bspwm")
          modules.emplace_back(new bspwm_module(bar, m_log, m_conf, module_name));
        else if (type == "internal/cpu")
          modules.emplace_back(new cpu_module(bar, m_log, m_conf, module_name));
        else if (type == "internal/date")
          modules.emplace_back(new date_module(bar, m_log, m_conf, module_name));
        else if (type == "internal/memory")
          modules.emplace_back(new memory_module(bar, m_log, m_conf, module_name));
        else if (type == "internal/i3")
          modules.emplace_back(new i3_module(bar, m_log, m_conf, module_name));
        else if (type == "internal/mpd")
          modules.emplace_back(new mpd_module(bar, m_log, m_conf, module_name));
        else if (type == "internal/volume")
          modules.emplace_back(new volume_module(bar, m_log, m_conf, module_name));
        else if (type == "internal/network")
          modules.emplace_back(new network_module(bar, m_log, m_conf, module_name));
        else if (type == "custom/text")
          modules.emplace_back(new text_module(bar, m_log, m_conf, module_name));
        else if (type == "custom/script")
          modules.emplace_back(new script_module(bar, m_log, m_conf, module_name));
        else if (type == "custom/menu")
          modules.emplace_back(new menu_module(bar, m_log, m_conf, module_name));
        else
          throw application_error("Unknown module: " + module_name);

        auto& module = modules.back();

        module->set_writer(bind(&controller::on_module_update, this, std::placeholders::_1));
        module->set_terminator(bind(&controller::on_module_stop, this, std::placeholders::_1));

        module_count++;
      }
    }

    if (module_count == 0)
      throw application_error("No modules created");
  }

  void on_module_update(string /* module_name */) {
    if (!m_mutex.try_lock_for(50ms)) {
      this_thread::yield();
      return;
    }
    std::lock_guard<std::timed_mutex> guard(m_mutex, std::adopt_lock);

    if (!m_running)
      return;
    if (!m_throttler->passthrough(m_throttle_strategy)) {
      m_log.trace("controller: Update event throttled");
      return;
    }

    string contents{""};
    string separator{m_bar->settings().separator};

    string padding_left(m_bar->settings().padding_left, ' ');
    string padding_right(m_bar->settings().padding_right, ' ');

    auto margin_left = m_bar->settings().module_margin_left;
    auto margin_right = m_bar->settings().module_margin_right;

    for (auto&& block : m_modules) {
      string block_contents;

      for (auto&& module : block.second) {
        auto module_contents = module->contents();

        if (module_contents.empty())
          continue;

        if (!block_contents.empty() && !separator.empty())
          block_contents += separator;

        if (!(block.first == alignment::LEFT && module == block.second.front()))
          block_contents += string(margin_left, ' ');

        block_contents += module->contents();

        if (!(block.first == alignment::RIGHT && module == block.second.back()))
          block_contents += string(margin_right, ' ');
      }

      if (block_contents.empty())
        continue;

      switch (block.first) {
        case alignment::LEFT:
          contents += "%{l}";
          contents += padding_left;
          break;
        case alignment::CENTER:
          contents += "%{c}";
          break;
        case alignment::RIGHT:
          contents += "%{r}";
          block_contents += padding_right;
          break;
        case alignment::NONE:
          break;
      }

      block_contents = string_util::replace_all(block_contents, "B-}%{B#", "B#");
      block_contents = string_util::replace_all(block_contents, "F-}%{F#", "F#");
      block_contents = string_util::replace_all(block_contents, "T-}%{T", "T");
      contents += string_util::replace_all(block_contents, "}%{", " ");
    }

    if (m_stdout)
      std::cout << contents << std::endl;
    else
      m_bar->parse(contents);
  }

  void on_module_stop(string /* module_name */) {
    if (!m_running)
      return;

    for (auto&& block : m_modules) {
      for (auto&& module : block.second) {
        if (module->running())
          return;
      }
    }

    m_log.warn("No running modules, raising SIGTERM");
    kill(getpid(), SIGTERM);
  }

  void on_module_click(string input) {
    if (!m_clickmtx.try_lock()) {
      this_thread::yield();
      return;
    }

    std::lock_guard<std::mutex> guard(m_clickmtx, std::adopt_lock);

    for (auto&& block : m_modules) {
      for (auto&& module : block.second) {
        if (!module->receive_events())
          continue;
        if (module->handle_event(input))
          return;
      }
    }

    m_log.trace("controller: Unrecognized input '%s'", input);
    m_log.trace("controller: Forwarding input to shell");

    auto command = command_util::make_command("/usr/bin/env\nsh\n-c\n" + input);

    try {
      command->exec(false);
      command->tail([this](std::string output) { m_log.trace("> %s", output); });
      command->wait();
    } catch (const application_error& err) {
      m_log.err(err.what());
    }
  }

 private:
  connection& m_connection;
  registry m_registry{m_connection};
  const logger& m_log;
  const config& m_conf;
  unique_ptr<bar> m_bar;
  unique_ptr<traymanager> m_traymanager;

  std::timed_mutex m_mutex;
  std::mutex m_clickmtx;

  stateflag m_stdout{false};
  stateflag m_running{false};
  stateflag m_reload{false};

  sigset_t m_waitmask;

  inotify_watch_t& m_confwatch;

  vector<thread> m_threads;
  map<alignment, vector<module_t>> m_modules;

  unique_ptr<throttle_util::event_throttler> m_throttler;
  throttle_util::strategy::try_once_or_leave_yolo m_throttle_strategy;
};

LEMONBUDDY_NS_END
