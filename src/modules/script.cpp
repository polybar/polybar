#include "modules/script.hpp"
#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<script_module>;

  /**
   * Construct script module by loading configuration values
   * and setting up formatting objects
   */
  script_module::script_module(const bar_settings& bar, string name_)
      : module<script_module>(bar, move(name_)), m_handler([&]() -> function<chrono::duration<double>()> {

        m_tail = m_conf.get(name(), "tail", false);
        // Handler for continuous tail commands {{{

        if (m_tail) {
          return [&] {
            if (!m_command || !m_command->is_running()) {
              string exec{string_util::replace_all(m_exec, "%counter%", to_string(++m_counter))};
              m_log.info("%s: Invoking shell command: \"%s\"", name(), exec);
              m_command = command_util::make_command(exec);

              try {
                m_command->exec(false);
              } catch (const exception& err) {
                m_log.err("%s: %s", name(), err.what());
                throw module_error("Failed to execute command, stopping module...");
              }
            }

            int fd = m_command->get_stdout(PIPE_READ);
            while (!m_stopping && fd != -1 && m_command->is_running() && !io_util::poll(fd, POLLHUP, 0)) {
              if (!io_util::poll_read(fd, 25)) {
                continue;
              } else if ((m_output = m_command->readline()) != m_prev) {
                m_prev = m_output;
                broadcast();
              }
            }

            if (m_stopping) {
              return chrono::duration<double>{0};
            } else if (m_command && !m_command->is_running()) {
              return std::max(m_command->get_exit_status() == 0 ? m_interval : 1s, m_interval);
            } else {
              return m_interval;
            }
          };
        }

        // }}}
        // Handler for basic shell commands {{{

        return [&] {
          try {
            auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_counter));
            m_log.info("%s: Invoking shell command: \"%s\"", name(), exec);
            m_command = command_util::make_command(exec);
            m_command->exec(true);
          } catch (const exception& err) {
            m_log.err("%s: %s", name(), err.what());
            throw module_error("Failed to execute command, stopping module...");
          }

          int fd = m_command->get_stdout(PIPE_READ);
          if (fd != -1 && io_util::poll_read(fd) && (m_output = m_command->readline()) != m_prev) {
            broadcast();
            m_prev = m_output;
          } else if (m_command->get_exit_status() != 0) {
            m_output.clear();
            m_prev.clear();
            broadcast();
          }

          return std::max(m_command->get_exit_status() == 0 ? m_interval : 1s, m_interval);
        };

        // }}}
      }()) {
    // Load configuration values
    m_exec = m_conf.get(name(), "exec", m_exec);
    m_exec_if = m_conf.get(name(), "exec-if", m_exec_if);
    m_interval = m_conf.get<decltype(m_interval)>(name(), "interval", 5s);

    // Load configured click handlers
    m_actions[mousebtn::LEFT] = m_conf.get(name(), "click-left", ""s);
    m_actions[mousebtn::MIDDLE] = m_conf.get(name(), "click-middle", ""s);
    m_actions[mousebtn::RIGHT] = m_conf.get(name(), "click-right", ""s);
    m_actions[mousebtn::DOUBLE_LEFT] = m_conf.get(name(), "double-click-left", ""s);
    m_actions[mousebtn::DOUBLE_MIDDLE] = m_conf.get(name(), "double-click-middle", ""s);
    m_actions[mousebtn::DOUBLE_RIGHT] = m_conf.get(name(), "double-click-right", ""s);
    m_actions[mousebtn::SCROLL_UP] = m_conf.get(name(), "scroll-up", ""s);
    m_actions[mousebtn::SCROLL_DOWN] = m_conf.get(name(), "scroll-down", ""s);

    // Setup formatting
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL});
    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), "label", "%output%");
    }
  }

  /**
   * Start the module worker
   */
  void script_module::start() {
    m_mainthread = thread([&] {
      try {
        while (running() && !m_stopping) {
          if (check_condition()) {
            sleep(process(m_handler));
          } else if (m_interval > 1s) {
            sleep(m_interval);
          } else {
            sleep(1s);
          }
        }
      } catch (const exception& err) {
        halt(err.what());
      }
    });
  }

  /**
   * Stop the module worker by terminating any running commands
   */
  void script_module::stop() {
    m_stopping = true;
    wakeup();

    std::lock_guard<decltype(m_handler)> guard(m_handler);

    m_command.reset();
    module::stop();
  }

  /**
   * Check if defined condition is met
   */
  bool script_module::check_condition() {
    if (m_exec_if.empty()) {
      return true;
    } else if (command_util::make_command(m_exec_if)->exec(true) == 0) {
      return true;
    } else if (!m_output.empty()) {
      broadcast();
      m_output.clear();
      m_prev.clear();
    }
    return false;
  }

  /**
   * Process mutex wrapped script handler
   */
  chrono::duration<double> script_module::process(const decltype(m_handler) & handler) const {
    std::lock_guard<decltype(handler)> guard(handler);
    return handler();
  }

  /**
   * Generate module output
   */
  string script_module::get_output() {
    if (m_output.empty()) {
      return "";
    }

    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%output%", m_output);
    }

    string cnt{to_string(m_counter)};
    string output{module::get_output()};

    for (auto btn : {mousebtn::LEFT, mousebtn::MIDDLE, mousebtn::RIGHT,
                     mousebtn::DOUBLE_LEFT, mousebtn::DOUBLE_MIDDLE,
                     mousebtn::DOUBLE_RIGHT, mousebtn::SCROLL_UP,
                     mousebtn::SCROLL_DOWN}) {

      auto action = m_actions[btn];

      if (!action.empty()) {
        auto action_replaced = string_util::replace_all(action, "%counter%", cnt);

        /*
         * The pid token is only for tailed commands.
         * If the command is not specified or running, replacement is unnecessary as well
         */
        if(m_tail && m_command && m_command->is_running()) {
          action_replaced = string_util::replace_all(action_replaced, "%pid%", to_string(m_command->get_pid()));
        }
        m_builder->cmd(btn, action_replaced);
      }
    }

    m_builder->append(output);

    return m_builder->flush();
  }

  /**
   * Output format tags
   */
  bool script_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else {
      return false;
    }

    return true;
  }
}

POLYBAR_NS_END
