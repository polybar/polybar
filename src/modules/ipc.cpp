#include "modules/ipc.hpp"

#include <unistd.h>

#include "components/ipc.hpp"
#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<ipc_module>;

  /**
   * Load user-defined ipc hooks and
   * create formatting tags
   */
  ipc_module::ipc_module(const bar_settings& bar, string name_)
      : module<ipc_module>(bar, move(name_)), m_current_hook(-1) {
    m_router->register_action_with_data(EVENT_SEND, [this](const std::string& data) { action_send(data); });
    m_router->register_action_with_data(EVENT_HOOK, [this](const std::string& data) { action_hook(data); });
    m_router->register_action(EVENT_NEXT, [this]() { action_next(); });
    m_router->register_action(EVENT_PREV, [this]() { action_prev(); });
    m_router->register_action(EVENT_RESET, [this]() { action_reset(); });

    size_t index = 0;

    for (auto&& command : m_conf.get_list<string>(name(), "hook", {})) {
      m_hooks.emplace_back(std::make_unique<hook>(hook{name() + to_string(++index), command}));
    }

    m_log.info("%s: Loaded %d hooks", name(), m_hooks.size());

    m_initial = m_conf.get(name(), "initial", 0_z);
    if (m_initial > 0 && m_initial <= m_hooks.size()) {
      m_current_hook = m_initial - 1;
    } else {
      throw module_error("Initial hook out of bounds '" + to_string(m_initial) + "'. Defined hooks goes from 1 to " +
                         to_string(m_hooks.size()) + ")");
    }

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

    const auto pid_token = [](const string& s) { return string_util::replace_all(s, "%pid%", to_string(getpid())); };

    for (auto& action : m_actions) {
      action.second = pid_token(action.second);
    }
    for (auto& hook : m_hooks) {
      hook->command = pid_token(hook->command);
    }

    m_formatter->add(DEFAULT_FORMAT, TAG_OUTPUT, {TAG_OUTPUT});
  }

  /**
   * Start module and run first defined hook if configured to
   */
  void ipc_module::start() {
    m_mainthread = thread([&] {
      m_log.trace("%s: Thread id = %i", this->name(), concurrency_util::thread_id(this_thread::get_id()));
      update();
      broadcast();
    });

    if (m_initial) {
      // TODO do this in a thread.
      auto command = command_util::make_command<output_policy::REDIRECTED>(m_hooks.at(m_initial - 1)->command);
      command->exec(false);
      command->tail([this](string line) { m_output = line; });
    }
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
        m_builder->action(action.first, action.second);
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
    for (size_t i = 0; i < m_hooks.size(); i++) {
      const auto& hook = m_hooks[i];
      if (hook->payload == message) {
        m_log.info("%s: Found matching hook (%s)", name(), hook->payload);
        m_current_hook = i;
        this->exec_hook();
        break;
      }
    }
  }

  void ipc_module::action_send(const string& data) {
    m_output = data;
    broadcast();
  }

  void ipc_module::action_hook(const string& data) {
    try {
      int hook = std::stoi(data);

      if (hook < 0 || (size_t)hook >= m_hooks.size()) {
        throw module_error("Hook action received with an out of bounds hook '" + to_string(hook) +
                           "'. Defined hooks goes from 0 to " + to_string(m_hooks.size() - 1) + ")");
      }
      m_current_hook = hook;
      this->exec_hook();
    } catch (const std::invalid_argument& err) {
      m_log.err("Failed to convert hook index to integer err: %s", err.what());
    }
  }

  void ipc_module::action_next() {
    m_current_hook = (m_current_hook + 1) % m_hooks.size();
    this->exec_hook();
  }

  void ipc_module::action_prev() {
    if (m_current_hook == 0) {
      m_current_hook = m_hooks.size();
    }
    m_current_hook--;
    this->exec_hook();
  }

  void ipc_module::action_reset() {
    if (m_initial != 0) {
      m_current_hook = m_initial - 1;
      this->exec_hook();
    } else {
      m_output.clear();
      broadcast();
    }
  }

  void ipc_module::exec_hook() {
    // Clear the output in case the command produces no output
    m_output.clear();

    try {
      auto command = command_util::make_command<output_policy::REDIRECTED>(m_hooks[m_current_hook]->command);
      command->exec(false);
      command->tail([this](string line) { m_output = line; });
    } catch (const exception& err) {
      m_log.err("%s: Failed to execute hook command (err: %s)", name(), err.what());
      m_output.clear();
    }

    broadcast();
  }
}  // namespace modules

POLYBAR_NS_END
