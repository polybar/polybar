#pragma once

#include "modules/meta.hpp"
#include "utils/command.hpp"

LEMONBUDDY_NS

#define SHELL_CMD "/usr/bin/env\nsh\n-c\n%cmd%"
#define OUTPUT_ACTION(BUTTON)     \
  if (!m_actions[BUTTON].empty()) \
  m_builder->cmd(BUTTON, string_util::replace_all(m_actions[BUTTON], "%counter%", counter_str))

namespace modules {
  class script_module : public event_module<script_module> {
   public:
    using event_module::event_module;

    void setup() {
      m_formatter->add(DEFAULT_FORMAT, TAG_OUTPUT, {TAG_OUTPUT});

      // Load configuration values

      REQ_CONFIG_VALUE(name(), m_exec, "exec");
      GET_CONFIG_VALUE(name(), m_tail, "tail");
      GET_CONFIG_VALUE(name(), m_maxlen, "maxlen");
      GET_CONFIG_VALUE(name(), m_ellipsis, "ellipsis");

      m_actions[mousebtn::LEFT] = m_conf.get<string>(name(), "click-left", "");
      m_actions[mousebtn::MIDDLE] = m_conf.get<string>(name(), "click-middle", "");
      m_actions[mousebtn::RIGHT] = m_conf.get<string>(name(), "click-right", "");
      m_actions[mousebtn::SCROLL_UP] = m_conf.get<string>(name(), "scroll-up", "");
      m_actions[mousebtn::SCROLL_DOWN] = m_conf.get<string>(name(), "scroll-down", "");

      m_interval = interval_t{m_conf.get<float>(name(), "interval", m_tail ? 0.0f : 2.0f)};
    }

    void stop() {
      wakeup();
      enable(false);
      m_command.reset();
      std::lock_guard<threading_util::spin_lock> lck(m_updatelock);
      wakeup();
    }

    void idle() {
      if (!enabled())
        sleep(100ms);
      if (!m_tail)
        sleep(m_interval);
      else if (!m_command || !m_command->is_running())
        sleep(m_interval);
    }

    bool has_event() {
      // Non tail commands should always run
      if (!m_tail)
        return true;

      if (!enabled())
        return false;

      try {
        if (!m_command || !m_command->is_running()) {
          auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_counter));
          m_log.trace("%s: Executing '%s'", name(), exec);

          m_command = command_util::make_command(string_util::replace(SHELL_CMD, "%cmd%", exec));
          m_command->exec(false);
        }
      } catch (const std::exception& err) {
        m_log.err("%s: %s", name(), err.what());
        throw module_error("Failed to execute tail command, stopping module...");
      }

      if (!m_command)
        return false;

      if ((m_output = m_command->readline()) == m_prev)
        return false;

      m_prev = m_output;

      return true;
    }

    bool update() {
      // Tailing commands always update
      if (m_tail)
        return true;

      try {
        auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_counter));
        auto cmd = command_util::make_command(string_util::replace(SHELL_CMD, "%cmd%", exec));

        m_log.trace("%s: Executing '%s'", name(), exec);

        cmd->exec(true);
        cmd->tail([&](string output) { m_output = output; });
      } catch (const std::exception& err) {
        m_log.err("%s: %s", name(), err.what());
        throw module_error("Failed to execute command, stopping module...");
      }

      if (m_output == m_prev)
        return false;

      m_prev = m_output;
      return true;
    }

    string get_output() {
      if (m_output.empty())
        return " ";

      // Truncate output to the defined max length
      if (m_maxlen > 0 && m_output.length() > m_maxlen) {
        m_output.erase(m_maxlen);
        m_output += m_ellipsis ? "..." : "";
      }

      auto counter_str = to_string(m_counter);
      OUTPUT_ACTION(mousebtn::LEFT);
      OUTPUT_ACTION(mousebtn::MIDDLE);
      OUTPUT_ACTION(mousebtn::RIGHT);
      OUTPUT_ACTION(mousebtn::SCROLL_UP);
      OUTPUT_ACTION(mousebtn::SCROLL_DOWN);
      m_builder->node(module::get_output());

      return m_builder->flush();
    }

    bool build(builder* builder, string tag) const {
      if (tag == TAG_OUTPUT) {
        builder->node(m_output);
        return true;
      } else {
        return false;
      }
    }

   protected:
    static constexpr auto TAG_OUTPUT = "<output>";

    command_util::command_t m_command;

    string m_exec;
    bool m_tail = false;
    interval_t m_interval = 0s;
    size_t m_maxlen = 0;
    bool m_ellipsis = true;
    map<mousebtn, string> m_actions;

    string m_output;
    string m_prev;
    int m_counter{0};
  };
}

LEMONBUDDY_NS_END
