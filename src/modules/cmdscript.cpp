#include "modules/cmdscript.hpp"
#include "drawtypes/label.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  cmdscript_module::cmdscript_module(const bar_settings& bar, string name_) : script_module(bar, move(name_)) {
    m_interval = m_conf.get<decltype(m_interval)>(name(), "interval", 5s);
  }

  void cmdscript_module::process() {
    if (!m_updatelock.try_lock()) {
      return;
    }

    try {
      std::unique_lock<mutex> guard(m_updatelock, std::adopt_lock);
      auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_counter));
      m_log.info("%s: Invoking shell command: \"%s\"", name(), exec);
      m_command = command_util::make_command(exec);
      m_command->exec(true);
    } catch (const exception& err) {
      m_log.err("%s: %s", name(), err.what());
      throw module_error("Failed to execute command, stopping module...");
    }

    if ((m_output = m_command->readline()) != m_prev) {
      broadcast();
      m_prev = m_output;
    }

    sleep(std::max(m_command->get_exit_status() == 0 ? m_interval : 1s, m_interval));
  }
}

POLYBAR_NS_END
