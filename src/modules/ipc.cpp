#include "modules/ipc.hpp"

#include <unistd.h>

#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<ipc_module>;

  /**
   * Load user-defined ipc hooks and
   * create formatting tags
   */
  ipc_module::ipc_module(const bar_settings& bar, string name_, const config& config)
      : module<ipc_module>(bar, move(name_), config) {
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

    // Negative initial values should always be -1
    m_initial = std::max(-1, m_conf.get(name(), "initial", 0) - 1);
    if (has_initial()) {
      if ((size_t)m_initial >= m_hooks.size()) {
        throw module_error("Initial hook out of bounds: '" + to_string(m_initial + 1) +
                           "'. Defined hooks go from 1 to " + to_string(m_hooks.size()) + ")");
      }
      m_current_hook = m_initial;
    } else {
      m_current_hook = -1;
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

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_OUTPUT});

    m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%output%");

    for (size_t i = 0; i < m_hooks.size(); i++) {
      string format_i = "format-" + to_string(i);
      m_formatter->add_optional(format_i, {TAG_LABEL});
    }
  }

  /**
   * Start module and run first defined hook if configured to
   */
  void ipc_module::start() {
    this->module::start();
    m_mainthread = thread([&] {
      m_log.trace("%s: Thread id = %i", this->name(), concurrency_util::thread_id(this_thread::get_id()));
      // Initial update to start with an empty output until the initial hook finishes
      update_output();
      update();
      broadcast();
    });
  }

  void ipc_module::update() {
    if (has_hook()) {
      exec_hook();
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
    if (output.empty()) {
      return "";
    }

    for (auto&& action : m_actions) {
      if (!action.second.empty()) {
        m_builder->action(action.first, action.second);
      }
    }

    m_builder->node(output);
    return m_builder->flush();
  }

  string ipc_module::get_format() const {
    if (m_current_hook != -1 && (size_t)m_current_hook < m_hooks.size()) {
      string format_i = "format-" + to_string(m_current_hook);
      if (m_formatter->has_format(format_i)) {
        return format_i;
      } else {
        return DEFAULT_FORMAT;
      }
    }
    return DEFAULT_FORMAT;
  }
  /**
   * Output content retrieved from hook commands
   */
  bool ipc_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL) {
      builder->node(m_label);
      return true;
    } else if (tag == TAG_OUTPUT) {
      builder->node(m_output);
      return true;
    }
    return false;
  }

  /**
   * Map received message hook to the ones
   * configured from the user config and
   * execute its command
   *
   * This code path is deprecated, all messages to ipc modules should go through actions.
   */
  void ipc_module::on_message(const string& message) {
    for (size_t i = 0; i < m_hooks.size(); i++) {
      const auto& hook = m_hooks[i];
      if (hook->payload == message) {
        m_log.info("%s: Found matching hook (%s)", name(), hook->payload);
        set_hook(i);
        break;
      }
    }
  }

  void ipc_module::action_send(const string& data) {
    m_output = data;
    update_output();
  }

  void ipc_module::action_hook(const string& data) {
    try {
      int hook = std::stoi(data);

      if (hook < 0 || (size_t)hook >= m_hooks.size()) {
        m_log.err("%s: Hook action received with an out of bounds hook '%s'. Defined hooks go from 0 to %zu.", name(),
            data, m_hooks.size() - 1);
      } else {
        set_hook(hook);
      }
    } catch (const std::invalid_argument& err) {
      m_log.err(
          "%s: Hook action received '%s' cannot be converted to a valid hook index. Defined hooks goes from 0 to "
          "%zu.",
          name(), data, m_hooks.size() - 1);
    }
  }

  void ipc_module::action_next() {
    hook_offset(1);
  }

  void ipc_module::action_prev() {
    hook_offset(-1);
  }

  void ipc_module::action_reset() {
    if (has_initial()) {
      set_hook(m_initial);
    } else {
      m_current_hook = -1;
      m_output.clear();

      update_output();
    }
  }

  /**
   * Changes the current hook by the given offset.
   *
   * Also deals with the case where the there is no active hook, in which case a positive offset starts from -1 and a
   * negative from 0. This ensures that 'next' executes the first and 'prev' the last hook if no hook is set.
   */
  void ipc_module::hook_offset(int offset) {
    int start_hook;

    if (has_hook()) {
      start_hook = m_current_hook;
    } else {
      if (offset > 0) {
        start_hook = -1;
      } else {
        start_hook = 0;
      }
    }

    set_hook((start_hook + offset + m_hooks.size()) % m_hooks.size());
  }

  bool ipc_module::has_initial() const {
    return m_initial >= 0;
  }

  bool ipc_module::has_hook() const {
    return m_current_hook >= 0;
  }

  void ipc_module::set_hook(int h) {
    assert(h >= 0 && (size_t)h < m_hooks.size());
    m_current_hook = h;
    exec_hook();
  }

  void ipc_module::exec_hook() {
    // Clear the output in case the command produces no output
    m_output.clear();

    try {
      command<output_policy::REDIRECTED> cmd(m_log, m_hooks[m_current_hook]->command);
      cmd.exec(false);
      cmd.tail([this](string line) { m_output = line; });
    } catch (const exception& err) {
      m_log.err("%s: Failed to execute hook command (err: %s)", name(), err.what());
      m_output.clear();
    }

    update_output();
  }

  void ipc_module::update_output() {
    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%output%", m_output);
    }
    broadcast();
  }
} // namespace modules

POLYBAR_NS_END
