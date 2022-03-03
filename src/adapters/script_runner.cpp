#include "adapters/script_runner.hpp"

#include <cassert>
#include <functional>

#include "modules/meta/base.hpp"
#include "utils/command.hpp"
#include "utils/io.hpp"
#include "utils/scope.hpp"
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
bool script_runner::check_condition() const {
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

int script_runner::get_exit_status() const {
  return m_exit_status;
}

string script_runner::get_output() {
  std::lock_guard<std::mutex> guard(m_output_lock);
  return m_output;
}

bool script_runner::is_stopping() const {
  return m_stopping;
}

/**
 * Updates the current output.
 *
 * Returns true if the output changed.
 */
bool script_runner::set_output(string&& new_output) {
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
    cmd->exec(false, m_env);
  } catch (const exception& err) {
    m_log.err("script_runner: %s", err.what());
    throw modules::module_error("Failed to execute command, stopping module...");
  }

  int fd = cmd->get_stdout(PIPE_READ);
  assert(fd != -1);

  bool changed = false;

  bool got_output = false;
  while (!m_stopping && cmd->is_running() && !io_util::poll(fd, POLLHUP, 0)) {
    /**
     * For non-tailed scripts, we only use the first line. However, to ensure interruptability when the module shuts
     * down, we still need to continue polling.
     */
    if (io_util::poll_read(fd, 25) && !got_output) {
      changed = set_output(cmd->readline());
      got_output = true;
    }
  }

  if (m_stopping) {
    cmd->terminate();
    return 0s;
  }

  m_exit_status = cmd->wait();

  if (!changed && m_exit_status != 0) {
    clear_output();
  }

  if (m_exit_status == 0) {
    return m_interval;
  } else {
    return std::max(m_interval, interval{1s});
  }
}

script_runner::interval script_runner::run_tail() {
  auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_counter));
  m_log.info("script_runner: Invoking shell command: \"%s\"", exec);
  auto cmd = command_util::make_command<output_policy::REDIRECTED>(exec);

  try {
    cmd->exec(false, m_env);
  } catch (const exception& err) {
    throw modules::module_error("Failed to execute command: " + string(err.what()));
  }

  auto pid_guard = scope_util::make_exit_handler([this]() { m_pid = -1; });
  m_pid = cmd->get_pid();

  int fd = cmd->get_stdout(PIPE_READ);
  assert(fd != -1);

  while (!m_stopping && cmd->is_running() && !io_util::poll(fd, POLLHUP, 0)) {
    if (io_util::poll_read(fd, 25)) {
      set_output(cmd->readline());
    }
  }

  if (m_stopping) {
    cmd->terminate();
    return 0s;
  }

  auto exit_status = cmd->wait();

  if (exit_status == 0) {
    return m_interval;
  } else {
    return std::max(m_interval, interval{1s});
  }
}

POLYBAR_NS_END
