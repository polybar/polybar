#include "modules/ipc.hpp"

#include "components/ipc.hpp"
#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<ipc_module>;

  /**
   * Load user-defined ipc hooks and
   * create formatting tags
   */
  ipc_module::ipc_module(const bar_settings& bar, string name_) : static_module<ipc_module>(bar, move(name_)) {
    m_router->register_action_with_data(EVENT_SEND, &ipc_module::action_send);

    size_t index = 0;

    for (auto&& command : m_conf.get_list<string>(name(), "hook", {})) {
      m_hooks.emplace_back(std::make_unique<hook>(hook{name() + to_string(++index), command}));
    }

    m_log.info("%s: Loaded %d hooks", name(), m_hooks.size());

    if ((m_initial = m_conf.get(name(), "initial", 0_z)) && m_initial > m_hooks.size()) {
      throw module_error("Initial hook out of bounds (defined: " + to_string(m_hooks.size()) + ")");
    }
    else{
      active_hook = m_initial;
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
      auto command = command_util::make_command<output_policy::REDIRECTED>(m_hooks.at(m_initial - 1)->command);
      command->exec(false);
      command->tail([this](string line) { m_output = line; });
    }
    static_module::start();
  }

  string ipc_module::replace_active_hook_token(string hook_command) {
    const char active_hook_token[] = "%active-hook%";
    const char next_hook_token[] = "%next%";
    const char prev_hook_token[] = "%prev%";

    string::size_type a = hook_command.find(active_hook_token);
    if(a != string::npos){
      hook_command.replace(a, 13, to_string(get_active()));
    }
    string::size_type n = hook_command.find(next_hook_token);
    if(n != string::npos){
      hook_command.replace(n, 6, to_string((get_active()+1)%m_hooks.size()+1));
    }
    string::size_type p = hook_command.find(prev_hook_token);
    if(p != string::npos){
      hook_command.replace(p, 6, to_string((get_active()-1)%m_hooks.size()+1));
    }
    return hook_command;
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
        m_builder->action(action.first, replace_active_hook_token(action.second));
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
    for (size_t i = 0; i<m_hooks.size(); i++) {
      auto&& hook = m_hooks[i];
      if (hook->payload != message) {
        continue;
      }

      m_log.info("%s: Found matching hook (%s)", name(), hook->payload);

      try {
        // Clear the output in case the command produces no output
        m_output.clear();
        set_active(i);
        auto command = command_util::make_command<output_policy::REDIRECTED>(hook->command);
        command->exec(false);
        command->tail([this](string line) { m_output = line; });
      } catch (const exception& err) {
        m_log.err("%s: Failed to execute hook command (err: %s)", err.what());
        m_output.clear();
      }

      broadcast();
    }
  }

  void ipc_module::action_send(const string& data) {
    m_output = data;
    broadcast();
  }

  void ipc_module::set_active(size_t current){
    active_mutex.lock();
    active_hook=current;
    active_mutex.unlock();
  }

  size_t ipc_module::get_active(){
    size_t active;
    active_mutex.lock();
    active=active_hook;
    active_mutex.unlock();
    return active;
  }

POLYBAR_NS_END
