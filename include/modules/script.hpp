#pragma once

#include "modules/meta.hpp"
#include "utils/command.hpp"

LEMONBUDDY_NS

#define SHELL_CMD "/usr/bin/env\nsh\n-c\n"

namespace modules {
  class script_module : public timer_module<script_module> {
   public:
    using timer_module::timer_module;

    void setup() {
      // Load configuration values

      m_exec = m_conf.get<string>(name(), "exec");
      m_tail = m_conf.get<bool>(name(), "tail", m_tail);

      m_maxlen = m_conf.get<size_t>(name(), "maxlen", 0);
      m_ellipsis = m_conf.get<bool>(name(), "ellipsis", m_ellipsis);

      if (m_tail)
        m_interval = 0s;
      else
        m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 1));

      m_actions[mousebtn::LEFT] = m_conf.get<string>(name(), "click-left", "");
      m_actions[mousebtn::MIDDLE] = m_conf.get<string>(name(), "click-middle", "");
      m_actions[mousebtn::RIGHT] = m_conf.get<string>(name(), "click-right", "");

      m_actions[mousebtn::SCROLL_UP] = m_conf.get<string>(name(), "scroll-up", "");
      m_actions[mousebtn::SCROLL_DOWN] = m_conf.get<string>(name(), "scroll-down", "");

      // Add formats and elements

      m_formatter->add(DEFAULT_FORMAT, TAG_OUTPUT, {TAG_OUTPUT});

      // Start a subthread tailing the script

      if (m_tail)
        dispatch_tailscript_runner();
    }

    bool update() {
      m_output.clear();

      if (m_tail) {
        m_output = tail_command();
      } else {
        m_output = read_output();
      }

      if (m_maxlen > 0 && m_output.length() > m_maxlen) {
        m_output.erase(m_maxlen);
        if (m_ellipsis)
          m_output += "...";
      }

      return enabled() && !m_output.empty();
    }

    string get_output() {
      if (m_output.empty())
        return "";

      auto counter_str = to_string(m_counter);

      if (!m_actions[mousebtn::LEFT].empty())
        m_builder->cmd(mousebtn::LEFT,
            string_util::replace_all(m_actions[mousebtn::LEFT], "%counter%", counter_str));
      if (!m_actions[mousebtn::MIDDLE].empty())
        m_builder->cmd(mousebtn::MIDDLE,
            string_util::replace_all(m_actions[mousebtn::MIDDLE], "%counter%", counter_str));
      if (!m_actions[mousebtn::RIGHT].empty())
        m_builder->cmd(mousebtn::RIGHT,
            string_util::replace_all(m_actions[mousebtn::RIGHT], "%counter%", counter_str));

      if (!m_actions[mousebtn::SCROLL_UP].empty())
        m_builder->cmd(mousebtn::SCROLL_UP,
            string_util::replace_all(m_actions[mousebtn::SCROLL_UP], "%counter%", counter_str));
      if (!m_actions[mousebtn::SCROLL_DOWN].empty())
        m_builder->cmd(mousebtn::SCROLL_DOWN,
            string_util::replace_all(m_actions[mousebtn::SCROLL_DOWN], "%counter%", counter_str));

      m_builder->node(module::get_output());

      return m_builder->flush();
    }

    bool build(builder* builder, string tag) {
      if (tag == TAG_OUTPUT)
        builder->node(string_util::replace_all(m_output, "\n", ""));
      else
        return false;
      return true;
    }

   protected:
    /**
     * Read line from tailscript
     */
    string tail_command() {
      int bytes_read = 0;

      if (!m_command)
        return "";
      if (io_util::poll_read(m_command->get_stdout(PIPE_READ), 100))
        return io_util::readline(m_command->get_stdout(PIPE_READ), bytes_read);
      return "";
    }

    /**
     * Execute command and read its output
     */
    string read_output() {
      string output;

      try {
        m_log.trace("%s: Executing command '%s'", name(), m_exec);

        auto cmd = command_util::make_command(
            SHELL_CMD + string_util::replace_all(m_exec, "%counter%", to_string(++m_counter)));

        cmd->exec(false);

        while (true) {
          int bytes_read = 0;
          string contents = io_util::readline(cmd->get_stdout(PIPE_READ), bytes_read);
          if (bytes_read <= 0)
            break;
          output += contents;
          output += "\n";
        }

        cmd->wait();
      } catch (const system_error& err) {
        m_log.err(err.what());
        return "";
      }

      return output;
    }

    /**
     * Run tail script in separate thread
     */
    void dispatch_tailscript_runner() {
      m_threads.emplace_back([this] {
        try {
          while (enabled() && (!m_command || !m_command->is_running())) {
            m_log.trace("%s: Executing command '%s'", name(), m_exec);
            m_command = command_util::make_command(
                SHELL_CMD + string_util::replace_all(m_exec, "%counter%", to_string(++m_counter)));
            m_command->exec(true);
          }
        } catch (const system_error& err) {
          m_log.err("Failed to create command (what: %s)", err.what());
        }
      });
    }

   private:
    static constexpr auto TAG_OUTPUT = "<output>";

    unique_ptr<command_util::command> m_command;

    map<mousebtn, string> m_actions;

    string m_exec;
    bool m_tail = false;
    string m_output;
    int m_counter{0};

    size_t m_maxlen = 0;
    bool m_ellipsis = true;
  };
}

LEMONBUDDY_NS_END
