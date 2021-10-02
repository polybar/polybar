#include "adapters/script_runner.hpp"

#include <functional>

#include "modules/meta/base.hpp"
#include "utils/command.hpp"
#include "utils/io.hpp"
#include "utils/string.hpp"

POLYBAR_NS

script_runner::script_runner(std::function<void(void)> on_update, const string& exec, const string& exec_if, bool tail,
    interval interval, const vector<pair<string, string>>& env)
    : m_log(logger::make())
    , m_on_update(on_update)
    , m_exec(exec)
    , m_exec_if(exec_if)
    , m_tail(tail)
    , m_interval(interval)
    , m_env(env) {}

/**
 * Check if defined condition is met
 */
bool script_runner::check_condition() {
  if (m_exec_if.empty()) {
    return true;
  }

  auto exec_if_cmd = command_util::make_command<output_policy::IGNORED>(m_exec_if);
  return exec_if_cmd->exec(true) == 0;
}

/**
 * Process mutex wrapped script handler
 */
script_runner::interval script_runner::process() {
  if (m_tail) {
    return run_tail();
  } else {
    return run();
  }
}

void script_runner::clear_output() {
  set_output("");
}

void script_runner::stop() {
  m_stopping = true;
}

int script_runner::get_pid() const {
  return m_pid;
}

int script_runner::get_counter() const {
  return m_counter;
}

string script_runner::get_output() {
  std::lock_guard<std::mutex> guard(m_output_lock);
  return m_output;
}

/**
 * Updates the current output.
 *
 * Returns true if the output changed.
 */
bool script_runner::set_output(const string&& new_output) {
  std::lock_guard<std::mutex> guard(m_output_lock);

  if (m_output != new_output) {
    m_output = std::move(new_output);
    m_on_update();
    return true;
  }

  return false;
}

script_runner::interval script_runner::run() {
  auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_counter));
  m_log.info("script_runner: Invoking shell command: \"%s\"", exec);
  auto cmd = command_util::make_command<output_policy::REDIRECTED>(exec);

  try {
    cmd->exec(true, m_env);
  } catch (const exception& err) {
    m_log.err("script_runner: %s", err.what());
    throw modules::module_error("Failed to execute command, stopping module...");
  }

  int status = cmd->get_exit_status();
  int fd = cmd->get_stdout(PIPE_READ);
  bool changed = fd != -1 && io_util::poll_read(fd) && set_output(cmd->readline());

  if (!changed && status != 0) {
    clear_output();
  }

  return std::max(status == 0 ? m_interval : 1s, m_interval);
}

// TODO
class AfterReturn {
 public:
  AfterReturn(std::function<void(void)> f) : m_f(f){};
  ~AfterReturn() {
    if (m_f) {
      m_f();
    }
  }

 private:
  std::function<void(void)> m_f;
};

script_runner::interval script_runner::run_tail() {
  auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_counter));
  m_log.info("script_runner: Invoking shell command: \"%s\"", exec);
  auto cmd = command_util::make_command<output_policy::REDIRECTED>(exec);

  try {
    cmd->exec(false, m_env);
  } catch (const exception& err) {
    m_log.err("script_runner: %s", err.what());
    throw modules::module_error("Failed to execute command, stopping module...");
  }

  AfterReturn pid_guard([this]() { m_pid = -1; });
  m_pid = cmd->get_pid();

  int fd = cmd->get_stdout(PIPE_READ);
  while (!m_stopping && fd != -1 && cmd->is_running() && !io_util::poll(fd, POLLHUP, 0)) {
    if (!io_util::poll_read(fd, 25)) {
      continue;
    } else {
      set_output(cmd->readline());
    }
  }

  if (m_stopping) {
    return interval{0};
  } else if (!cmd->is_running()) {
    return std::max(cmd->get_exit_status() == 0 ? m_interval : 1s, m_interval);
  } else {
    return m_interval;
  }
}

POLYBAR_NS_END
