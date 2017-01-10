#include "modules/tailscript.hpp"
#include "drawtypes/label.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  tailscript_module::tailscript_module(const bar_settings& bar, string name_) : script_module(bar, move(name_)) {
    m_interval = m_conf.get<decltype(m_interval)>(name(), "interval", 0s);
  }

  void tailscript_module::process() {
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

    if (io_util::poll(m_command->get_stdout(PIPE_READ), POLLIN, 0)) {
      if ((m_output = m_command->readline()) != m_prev) {
        m_prev = m_output;
        broadcast();
      }
    }
  }

  chrono::duration<double> tailscript_module::sleep_duration() {
    if (m_command && !m_command->is_running()) {
      return std::max(m_command->get_exit_status() == 0 ? m_interval : 1s, m_interval);
    } else {
      return m_interval;
    }
  }
}

POLYBAR_NS_END
