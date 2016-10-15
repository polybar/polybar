#pragma once

#include "modules/meta.hpp"
#include "utils/command.hpp"

LEMONBUDDY_NS

#define SHELL_CMD "/usr/bin/env\nsh\n-c\n"
#define OUTPUT_ACTION(BUTTON)     \
  if (!m_actions[BUTTON].empty()) \
  m_builder->cmd(BUTTON, string_util::replace_all(m_actions[BUTTON], "%counter%", counter_str))

namespace modules {
  class script_module : public event_module<script_module> {
   public:
    using event_module::event_module;

    void setup() {
      m_formatter->add(DEFAULT_FORMAT, TAG_OUTPUT, {TAG_OUTPUT});

      // Load configuration values {{{
      REQ_CONFIG_VALUE(name(), m_exec, "exec");
      GET_CONFIG_VALUE(name(), m_tail, "tail");
      GET_CONFIG_VALUE(name(), m_maxlen, "maxlen");
      GET_CONFIG_VALUE(name(), m_ellipsis, "ellipsis");

      m_actions[mousebtn::LEFT] = m_conf.get<string>(name(), "click-left", "");
      m_actions[mousebtn::MIDDLE] = m_conf.get<string>(name(), "click-middle", "");
      m_actions[mousebtn::RIGHT] = m_conf.get<string>(name(), "click-right", "");
      m_actions[mousebtn::SCROLL_UP] = m_conf.get<string>(name(), "scroll-up", "");
      m_actions[mousebtn::SCROLL_DOWN] = m_conf.get<string>(name(), "scroll-down", "");

      if (!m_tail) {
        m_interval = interval_t{m_conf.get<float>(name(), "interval", m_interval.count())};
      }
      // }}}
      // Execute the tail command {{{
      if (m_tail) {
        try {
          auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_counter));
          m_log.trace("%s: Executing '%s'", name(), exec);

          m_command = command_util::make_command(SHELL_CMD + exec);
          m_command->exec(false);
        } catch (const std::exception& err) {
          m_log.err("%s: Failed to execute tail command, stopping module..", name());
          m_log.err("%s: %s", name(), err.what());
          stop();
        }
      }
      // }}}
    }

    void stop() {
      // Put the module in stopped state {{{
      event_module::stop();
      // }}}
      // Terminate running command {{{
      try {
        if (m_command)
          m_command.reset();
      } catch (const std::exception& err) {
        m_log.err("%s: %s", name(), err.what());
      }
      // }}}
    }

    bool has_event() {
      // Handle non-tailing command {{{
      if (!m_tail) {
        sleep(m_interval);
        return enabled();
      }
      // }}}
      // Handle tailing command {{{
      if (!m_command || !m_command->is_running()) {
        m_log.warn("%s: Tail command finished, stopping module...", name());
        stop();
        return false;
      } else if ((m_output = m_command->readline()) != m_prev) {
        m_prev = m_output;
        return true;
      } else {
        return false;
      }
      // }}}
    }

    bool update() {
      // Handle tailing command {{{
      if (m_tail)
        return true;
      // }}}
      // Handle non-tailing command {{{
      try {
        auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_counter));
        auto cmd = command_util::make_command(SHELL_CMD + exec);

        m_log.trace("%s: Executing '%s'", name(), exec);

        cmd->exec();
        cmd->tail([this](string contents) { m_output = contents; });
      } catch (const std::exception& err) {
        m_log.err("%s: Failed to execute command, stopping module..", name());
        m_log.err("%s: %s", name(), err.what());
        stop();
        return false;
      }

      if (m_output != m_prev) {
        m_prev = m_output;
        return true;
      }

      return false;
      // }}}
    }

    string get_output() {
      auto output = string_util::replace_all(m_output, "\n", "");
      if (output.empty())
        return " ";

      // Truncate output to the defined max length {{{
      if (m_maxlen > 0 && output.length() > m_maxlen) {
        output.erase(m_maxlen);
        output += m_ellipsis ? "..." : "";
      }
      // }}}
      // Add mousebtn command handlers {{{
      auto counter_str = to_string(m_counter);

      OUTPUT_ACTION(mousebtn::LEFT);
      OUTPUT_ACTION(mousebtn::MIDDLE);
      OUTPUT_ACTION(mousebtn::RIGHT);
      OUTPUT_ACTION(mousebtn::SCROLL_UP);
      OUTPUT_ACTION(mousebtn::SCROLL_DOWN);
      // }}}

      m_builder->node(output);

      return m_builder->flush();
    }

    bool build(builder* builder, string tag) {
      if (tag != TAG_OUTPUT)
        return false;
      builder->node(m_output);
      return true;
    }

   protected:
    static constexpr auto TAG_OUTPUT = "<output>";

    command_util::command_t m_command;

    string m_exec;
    bool m_tail = false;
    interval_t m_interval = 1s;
    size_t m_maxlen = 0;
    bool m_ellipsis = true;
    map<mousebtn, string> m_actions;

    string m_output;
    string m_prev;
    int m_counter{0};
  };
}

LEMONBUDDY_NS_END
