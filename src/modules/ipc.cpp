#include "components/ipc.hpp"
#include "modules/ipc.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<ipc_module>;

  /**
   * Load user-defined ipc hooks and
   * create formatting tags
   */
  ipc_module::ipc_module(const bar_settings& bar, string name_) : module<ipc_module>(bar, move(name_)) {
    size_t index = 0;

    for (auto&& command : m_conf.get_list<string>(name(), "hook")) {
      m_hooks.emplace_back(new hook{name() + to_string(++index), command, 0_z});
    }

    for (size_t i = 0; i < m_hooks.size(); ++i) {
      m_hooks[i]->interval = m_conf.get(name(), "hook-" + to_string(i) + "-interval", 0_z);
    }

    if (m_hooks.empty()) {
      throw module_error("No hooks defined");
    }

    if ((m_initial = m_conf.get(name(), "initial", 0_z)) && m_initial > m_hooks.size()) {
      throw module_error("Initial hook out of bounds (defined: " + to_string(m_hooks.size()) + ")");
    }

    m_current = m_initial;

    // clang-format off
    m_actions.emplace(make_pair<mousebtn, string>(mousebtn::LEFT, m_conf.get(name(), "click-left", ""s)));
    m_actions.emplace(make_pair<mousebtn, string>(mousebtn::MIDDLE, m_conf.get(name(), "click-middle", ""s)));
    m_actions.emplace(make_pair<mousebtn, string>(mousebtn::RIGHT, m_conf.get(name(), "click-right", ""s)));
    m_actions.emplace(make_pair<mousebtn, string>(mousebtn::SCROLL_UP, m_conf.get(name(), "scroll-up", ""s)));
    m_actions.emplace(make_pair<mousebtn, string>(mousebtn::SCROLL_DOWN, m_conf.get(name(), "scroll-down", ""s)));
    m_actions.emplace(make_pair<mousebtn, string>(mousebtn::DOUBLE_LEFT, m_conf.get(name(), "double-click-left", ""s)));
    m_actions.emplace(make_pair<mousebtn, string>(mousebtn::DOUBLE_MIDDLE, m_conf.get(name(), "double-click-middle", ""s)));
    m_actions.emplace(make_pair<mousebtn, string>(mousebtn::DOUBLE_RIGHT, m_conf.get(name(), "double-click-right", ""s)));
    // clang-format on

    const auto pid_token = [](string& s) {
      string::size_type p = s.find("%pid%");
      if (p != string::npos) {
        s.replace(p, 5, to_string(getpid()));
      }
    };

    for (auto& action : m_actions) {
      pid_token(action.second);
    }
    for (auto& hook : m_hooks) {
      pid_token(hook->command);
    }

    m_formatter->add(DEFAULT_FORMAT, TAG_OUTPUT, {TAG_OUTPUT});
  }

  /**
   * Start module and run first defined hook if configured to
   */
  void ipc_module::start() {
    if (m_initial) {
      auto command = command_util::make_command(m_hooks.at(m_initial - 1)->command);
      command->exec(false);
      command->tail([this](string line) { m_output = line; });
    }
    m_mainthread = thread(&ipc_module::runner, this);
  }

  bool ipc_module::update() {
    if (m_current && m_hooks.at(m_current - 1)->interval) {
      auto command = command_util::make_command(m_hooks[m_current - 1]->command);
      command->exec(false);
      command->tail([this](string line) { m_output = line; });

      return true;
    }

    return false;
  }

  /**
   * Wrap the output with defined mouse actions
   */
  string ipc_module::get_output() {
    // Get the module output early so that
    // the format prefix/suffix also gets wrapper
    // with the cmd handlers
    string output{module::get_output()};

    for (auto&& action : m_actions) {
      if (!action.second.empty()) {
        m_builder->cmd(action.first, action.second);
      }
    }

    m_builder->append(output);
    return m_builder->flush();
  }

  /**
   * Output content retrieved from hook commands
   */
  bool ipc_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_OUTPUT) {
      builder->node(m_output);
      return true;
    } else {
      return false;
    }
  }

  /**
   * Map received message hook to the ones
   * configured from the user config and
   * execute its command
   */
  void ipc_module::on_message(const string& message) {
    size_t i = 0;
    for (auto&& hook : m_hooks) {
      ++i;
      if (hook->payload != message) {
        continue;
      }

      // Lock to avoid concurrency issues with ipc_module::update()
      m_updatelock.lock();

      m_log.info("%s: Found matching hook (%s)", name(), hook->payload);

      try {
        auto command = command_util::make_command(hook->command);
        command->exec(false);
        command->tail([this](string line) { m_output = line; });
      } catch (const exception& err) {
        m_log.err("%s: Failed to execute hook command (err: %s)", err.what());
        m_output.clear();
      }

      broadcast();

      m_current = i;
      // No need to wakeup the thread if hook interval is 0
      if (hook->interval) {
        m_updatelock.unlock();  // Must be unlocked before the wakeup
        wakeup();
      } else {
        m_updatelock.unlock();
      }
    }
  }

  void ipc_module::runner() {
    this->m_log.trace("%s: Thread id = %i", this->name(), concurrency_util::thread_id(this_thread::get_id()));

    const auto check = [this]() -> bool {
      std::unique_lock<std::mutex> guard(this->m_updatelock);
      return update();
    };

    try {
      // warm up module output before entering the loop
      check();
      broadcast();

      while (this->running()) {
        if (check()) {
          broadcast();
        }

        if (m_current && m_hooks[m_current - 1]->interval) {
          sleep(chrono::seconds(m_hooks[m_current - 1]->interval));
        } else {
          // No need to let the thread run if the current hook have an interval of 0.
          // The on_message method will wakeup this thread if the activated hook as an interval != 0.
          std::unique_lock<std::mutex> guard(m_sleeplock);
          m_sleephandler.wait(guard, [this]() { return m_should_wake_up; });
          m_should_wake_up = false;
        }
      }
    } catch (const exception& err) {
      halt(err.what());
    }
  }

  void ipc_module::wakeup() {
    m_should_wake_up = true;
    module<ipc_module>::wakeup();
  }
}  // namespace modules

POLYBAR_NS_END
