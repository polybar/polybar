#pragma once

#include "common.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "utils/file.hpp"

POLYBAR_NS

DEFINE_ERROR(command_error);

enum class output_policy {
  REDIRECTED,
  IGNORED,
};

/**
 * Wrapper used to execute command in a subprocess.
 * In-/output streams are opened to enable ipc.
 * If the command is created using command<output_policy::REDIRECTED>, the child streams are
 * redirected and you can read the program output or write into the program input.
 *
 * If the command is created using command<output_policy::IGNORED>, the output is not redirected and
 * you can't communicate with the child program.
 *
 * Example usage:
 *
 * @code cpp
 *   command<output_policy::REDIRECTED>auto(m_log, "cat /etc/rc.local");
 *   cmd->exec();
 *   cmd->tail([](string s) { std::cout << s << std::endl; });
 * @endcode
 *
 * @code cpp
 *   command<output_policy::REDIRECTED>auto(m_log, "for i in 1 2 3; do echo $i; done");
 *   cmd->exec();
 *   cout << cmd->readline(); // 1
 *   cout << cmd->readline() << cmd->readline(); // 23
 * @endcode
 *
 * @code cpp
 *   command<output_policy::IGNORED>auto(m_log, "ping kernel.org");
 *   int status = cmd->exec();
 * @endcode
 */
template <output_policy>
class command;

template <>
class command<output_policy::IGNORED> {
 public:
  explicit command(const logger& logger, string cmd);
  command(const command&) = delete;
  ~command();

  command& operator=(const command&) = delete;

  int exec(bool wait_for_completion = true);
  void terminate();
  bool is_running();
  int wait();

  pid_t get_pid();
  int get_exit_status();

 protected:
  const logger& m_log;

  string m_cmd;

  pid_t m_forkpid{-1};
  int m_forkstatus{-1};
};

template <>
class command<output_policy::REDIRECTED> : private command<output_policy::IGNORED> {
 public:
  explicit command(const logger& logger, string cmd);
  command(const command&) = delete;
  ~command();

  command& operator=(const command&) = delete;

  int exec(bool wait_for_completion = true, const vector<pair<string, string>>& env = {});
  using command<output_policy::IGNORED>::terminate;
  using command<output_policy::IGNORED>::is_running;
  using command<output_policy::IGNORED>::wait;

  using command<output_policy::IGNORED>::get_pid;
  using command<output_policy::IGNORED>::get_exit_status;

  void tail(std::function<void(string)> cb);
  string readline();
  bool wait_for_data(int timeout_ms);

  int get_stdout(int c);
  int get_stdin(int c);

 protected:
  int m_stdout[2]{0, 0};
  int m_stdin[2]{0, 0};

  unique_ptr<fd_stream<std::istream>> m_stdout_reader{nullptr};
};

POLYBAR_NS_END
