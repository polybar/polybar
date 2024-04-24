#include "adapters/script_runner.hpp"

#include <cassert>
#include <functional>

#include "modules/meta/base.hpp"
#include "utils/command.hpp"
#include "utils/io.hpp"
#include "utils/scope.hpp"
#include "utils/string.hpp"

POLYBAR_NS

script_runner::script_runner(on_update on_update, const string& exec, const string& exec_if, bool tail,
    interval interval_success, interval interval_fail, const vector<pair<string, string>>& env)
    : m_log(logger::make())
    , m_on_update(on_update)
    , m_exec(exec)
    , m_exec_if(exec_if)
    , m_tail(tail)
    , m_interval_success(interval_success)
    , m_interval_fail(interval_fail)
    , m_env(env) {}

/**
 * Check if defined condition is met
 */
bool script_runner::check_condition() const {
  if (m_exec_if.empty()) {
    return true;
  }

  command<output_policy::IGNORED> exec_if_cmd(m_log, m_exec_if);
  return exec_if_cmd.exec(true) == 0;
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
  auto changed = set_output("");
  if (changed) {
    m_on_update(m_data);
  }
}

void script_runner::stop() {
  m_stopping = true;
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
  if (m_data.output != new_output) {
    m_data.output = std::move(new_output);
    return true;
  }

  return false;
}

/**
 * Updates the current exit status
 *
 * Returns true if the exit status changed.
 */
bool script_runner::set_exit_status(int new_status) {
  auto changed = (m_data.exit_status != new_status);
  m_data.exit_status = new_status;

  return changed;
}

script_runner::interval script_runner::run() {
  auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_data.counter));
  m_log.info("script_runner: Invoking shell command: \"%s\"", exec);
  command<output_policy::REDIRECTED> cmd(m_log, exec);

  try {
    cmd.exec(false, m_env);
  } catch (const exception& err) {
    m_log.err("script_runner: %s", err.what());
    throw modules::module_error("Failed to execute command, stopping module...");
  }

  int fd = cmd.get_stdout(PIPE_READ);
  assert(fd != -1);

  bool changed = false;

  bool got_output = false;
  while (!m_stopping && cmd.is_running() && !io_util::poll(fd, POLLHUP, 0)) {
    /**
     * For non-tailed scripts, we only use the first line. However, to ensure interruptability when the module shuts
     * down, we still need to continue polling.
     */
    if (io_util::poll_read(fd, 250) && !got_output) {
      changed = set_output(cmd.readline());
      got_output = true;
    }
  }

  if (m_stopping) {
    cmd.terminate();
    return 0s;
  }

  auto exit_status_changed = set_exit_status(cmd.wait());

  if (changed || exit_status_changed) {
    m_on_update(m_data);
  }

  if (m_data.exit_status == 0) {
    return m_interval_success;
  } else {
    return std::max(m_interval_fail, interval{1s});
  }
}

script_runner::interval script_runner::run_tail() {
  auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_data.counter));
  m_log.info("script_runner: Invoking shell command: \"%s\"", exec);
  command<output_policy::REDIRECTED> cmd(m_log, exec);

  try {
    cmd.exec(false, m_env);
  } catch (const exception& err) {
    throw modules::module_error("Failed to execute command: " + string(err.what()));
  }

  scope_util::on_exit pid_guard([this]() {
    m_data.pid = -1;
    m_on_update(m_data);
  });

  m_data.pid = cmd.get_pid();

  int fd = cmd.get_stdout(PIPE_READ);
  assert(fd != -1);

  while (!m_stopping && cmd.is_running() && !io_util::poll(fd, POLLHUP, 0)) {
    if (cmd.wait_for_data(250)) {
      auto changed = set_output(cmd.readline());

      if (changed) {
        m_on_update(m_data);
      }
    }
  }

  if (m_stopping) {
    cmd.terminate();
    return 0s;
  }

  auto exit_status = cmd.wait();

  if (exit_status == 0) {
    return m_interval_success;
  } else {
    return std::max(m_interval_fail, interval{1s});
  }
}

POLYBAR_NS_END
